# Devlog #01 — Transport UDP Multi-Clients & Préparation Network Logic

| Métadonnées | Détails |
| :--- | :--- |
| **Date** | `2026-11-13` |
| **Heure (Début → Fin)** | `13:00` → `16:30` |
| **Temps passé** | `3h 30m` |
| **Branche / Commit** | `network-logic` (`65dbcf3`) |
| **Statut de fin de session** | 🟡 En cours (WIP) |

---

## 1. Contexte & Objectifs de la session
> *Où j'en étais en ouvrant le projet aujourd'hui, et qu'est-ce que je voulais accomplir pendant cette session ?*

En reprenant après la finalisation de la branche `interface-networld-godot` / `dev` (`ae905f5`) — qui a terminé le nettoyage de `NetEntity` dans `register_types.cpp`, branché `create_transport()` dans `NetWorld::host()`/`join()` et exposé `set_entity_velocity` / `set_entity_position` — l'objectif sur la nouvelle branche `network-logic` est de faire passer le réseau d'un lien 1-à-1 hardcodé à une vraie logique multi-clients.

- [x] Finaliser le nettoyage post-`NetEntity` et l'exposition de vélocité/position (`ae905f5`)
- [x] Gérer plusieurs clients par ID virtuel et le timeout côté `GodotUdpTransport` (`65dbcf3`)
- [ ] Gérer les événements `NetEvent` et le dispatch/sync/spawn dans `NetWorldCore`

---

## 2. Ce qui a été fait (Résumé des changements)
> *Liste claire des fonctionnalités ajoutées, modifiées ou supprimées.*

### Fichiers touchés
- `src/core/INetTransport.h` : ajout des constantes `PEER_BROADCAST = 0` et `PEER_SERVER = 1`, de l'enum `NetEvent` (`CONNECTED`, `DISCONNECTED`, `DATA`) et du champ `event` dans `NetPacket`.
- `src/godot/GodotUDPTransport.h` : ajout de `PEER_TIMEOUT = 5000` (ms), de la structure `ClientInfo` (`peer_id`, `ip`, `port`, `last_seen_ms`), des maps `clients_by_id` / `clients_to_id`, de `next_peer_id = 2` et de `make_endpoint_key()`.
- `src/godot/GodotUDPTransport.cpp` : implémentation de l'identification des clients par `ip:port`, génération des paquets `CONNECTED` / `DISCONNECTED` dans `rx_queue`, détection des timeouts dans `poll()`, et routage `send()` (client → serveur, serveur → `PEER_BROADCAST`, serveur → client ciblé).
- *(En amont sur `dev` / `interface-networld-godot`)* : `src/godot/register_types.cpp` (`606bea1`), `demo/test_networld.tscn` (`feffe80`), `src/godot/Networld.h` & `src/godot/Networld.cpp` (`ae905f5`).

### Explication du fonctionnement
- **Identification virtuelle des pairs :** Dans `GodotUdpTransport::poll()`, lorsqu'un paquet arrive côté hôte, l'endpoint `ip:port` est converti en clé via `make_endpoint_key()`. Si la clé est inconnue dans `clients_to_id`, un `peer_id` incrémental (à partir de `2`, `1` étant réservé à `PEER_SERVER`) est attribué, un `NetPacket` interne avec `event = NetEvent::CONNECTED` est poussé dans `rx_queue`, puis le paquet de données suit.
- **Détection de déconnexion (timeout) :** À chaque `poll()` côté hôte, `last_seen_ms` est vérifié par rapport à `Time::get_singleton()->get_ticks_msec()`. Tout client silencieux depuis plus de `5000 ms` est retiré des deux maps et un paquet `NetEvent::DISCONNECTED` est poussé dans `rx_queue`.
- **Envoi ciblé & Broadcast :** `GodotUdpTransport::send()` gère maintenant trois cas : client vers `target_ip:target_port`, hôte en `PEER_BROADCAST` (`0`) qui itère sur `clients_by_id`, et hôte vers un `target_peer_id` spécifique.

---

## 3. Choix Techniques & Architecture (Le "Pourquoi")
> *Pourquoi avoir codé ça de cette manière plutôt qu'une autre ? Quels compromis (performance, lisibilité, découplage) ont été faits ?*

- **Décision 1 : Remonter `CONNECTED` / `DISCONNECTED` via `rx_queue` sous forme de `NetPacket` (`NetEvent`)**
  - *Alternative envisagée :* Ajouter des callbacks séparés ou des méthodes dédiées sur `INetTransport`.
  - *Raison du choix :* Unifier tout ce qui entre dans `NetWorldCore::step()` dans une seule boucle `while (transport_->recv(pkt))` pour garder `INetTransport` minimaliste et préparer un dispatcher centralisé.
- **Décision 2 : Double indexation `clients_by_id` + `clients_to_id` (`ip:port`)**
  - *Alternative envisagée :* Parcourir linéairement une liste de clients à chaque paquet reçu ou envoyé.
  - *Raison du choix :* Accès en $O(1)$ dans `poll()` (depuis `ip:port`) et dans `send()` (depuis `target_peer_id`).

---

## 4. Problèmes rencontrés & Solutions (Debugging)
> *Qu'est-ce qui a posé problème ou fait perdre du temps aujourd'hui, et comment ça a été résolu ?*

- Aucun problème bloquant rencontré durant cette session.

---

## 5. Où j'en suis & Prochaine Session (Reprise rapide)
> *LA section à relire en premier la prochaine fois que j'ouvre le projet !*

- **État exact du code en quittant :**
  - Le transport `GodotUdpTransport` gère le multi-clients, le broadcast et les timeouts.
  - **Attention :** `NetWorldCore::step()` dans `src/core/NetworldCore.cpp` appelle encore directement `deserialize_buffer(pkt.payload.data(), pkt.payload.size())` dans `while (transport_->recv(pkt))` sans vérifier `pkt.event`. Un paquet `CONNECTED`/`DISCONNECTED` (payload vide) ou le `knock_packet` (`0xFF`) passe donc encore dans le désérialiseur SoA.
- **Prochaines étapes immédiates (TODO de reprise) :**
  1. [ ] Dans `src/core/NetworldCore.cpp`, brancher un switch/dispatch sur `pkt.event` (`CONNECTED`, `DISCONNECTED`, `DATA`) dans `step()`.
  2. [ ] Définir un en-tête / type de message pour distinguer `knock`, `spawn`, `despawn` et `state_sync` (batches SoA) dans `NetPacket::payload`.
  3. [ ] Implémenter la logique de spawn/despawn et de synchronisation initiale lors de la connexion (`NetEvent::CONNECTED`) et déconnexion (`NetEvent::DISCONNECTED`) d'un client.
- **Idées / Questions mises de côté pour plus tard :**
  - Gestion de la fiabilité pour `NetChannel::RELIABLE_EVENTS` (actuellement UDP brut sans ACK/resend).
