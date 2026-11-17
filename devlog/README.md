# Journal de Développement (Devlog) — Networld

Bienvenue dans le dossier **Devlog** de **Networld**. Ce dossier te permet de garder une trace claire de toute l'architecture du projet et de savoir exactement où tu t'es arrêté à chaque session de code.

---

## 📌 Point de Reprise Rapide (À mettre à jour en fin de session)

- **Dernière mise à jour :** `2026-11-16` (Branche `network-logic`, Commit `fa09e14` — *UPDATE Networld*)
- **État actuel :** Définition des messages réseau typés ([NetMessage.h](file:///c:/Users/leoul/Godot/Projects/Networld/src/core/NetMessage.h) : `KnockMsg`, `SpawnEntityMsg`, `DespawnEntityMsg`, `StateSyncMsg`), câblage complet de `PacketDispatcher` et `NetEvent` dans [NetworldCore.cpp](file:///c:/Users/leoul/Godot/Projects/Networld/src/core/NetworldCore.cpp), mapping d'IDs Flecs (`peer_entities_` / `net_to_local_entities_`), et liaison dynamique avec Godot via les signaux `entity_spawned` / `entity_despawned` et `bind_entity_view`. Le spawn et la synchronisation de mouvement fonctionnent entre Host et Client.
- **À faire immédiatement à la reprise :**
  1. [ ] Ajouter un keepalive (`KnockMsg`) côté client dans la branche `else` de l'egress de `NetWorldCore::step()` (ex. `if (current_tick_ % 60 == 0)`) pour éviter le `PEER_TIMEOUT` (`5000 ms`) dans `GodotUdpTransport` et permettre la réception de `DespawnEntityMsg` par les clients actifs.
  2. [ ] Recompiler et tester avec 1 Host + 1 ou 2 Clients pour valider la persistance et la propagation du despawn.
  3. [ ] Nettoyer les derniers détails (`= 0` sur `DespawnEntityMsg::entity_Id` et `StateSyncMsg::tick`, code commenté dans `step()`, getters `const` pour `spawned_entities_` / `despawned_entities_`, `TODO` dans `bind_entity_view`).
  4. [ ] Mettre en place l'envoi d'inputs / contrôle de l'entité joueur côté client.

---

## 📚 Index des Documents

| Fichier | Description |
| :--- | :--- |
| **[00_ARCHITECTURE_COMPLETE.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/00_ARCHITECTURE_COMPLETE.md)** | **Bilan complet du projet (2 Nov → 12 Nov 2026)** : Architecture `Core` vs `Godot`, explication détaillée de chaque fichier, schémas Mermaid (Flux, Sérialisation SoA, Boucle `step`), et choix techniques. |
| **[01_2026-11-13_transport_multi_clients.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/01_2026-11-13_transport_multi_clients.md)** | **Session #01 (13 Nov 2026)** : Transport UDP multi-clients par `peer_id`, broadcast, timeouts et événements `NetEvent` sur `network-logic`. |
| **[02_2026-11-14_stream_et_packet_dispatcher.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/02_2026-11-14_stream_et_packet_dispatcher.md)** | **Session #02 (14 Nov 2026)** : Migration C++20 (`SConstruct`), concepts (`NetStream.h`), `StreamWriter`/`StreamReader` et `PacketDispatcher`. |
| **[03_2026-11-16_netmessage_et_sync_entites.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/03_2026-11-16_netmessage_et_sync_entites.md)** | **Session #03 (16 Nov 2026)** : Messages `NetMessage.h`, câblage `PacketDispatcher`/`NetEvent` dans `NetWorldCore`, mapping d'IDs Flecs et signaux Godot (`entity_spawned`/`entity_despawned`). |
| **[TEMPLATE_DEVLOG.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/TEMPLATE_DEVLOG.md)** | **Modèle de session** à copier-coller à chaque nouvelle session de code (`01_YYYY-MM-DD_sujet.md`, `02_...`). |

---

## 🗂️ Historique des Sessions

| Session | Date | Durée | Sujet principal | Statut |
| :--- | :--- | :--- | :--- | :--- |
| **[#00](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/00_ARCHITECTURE_COMPLETE.md)** | `2026-11-02` → `2026-11-12` | ~10 jours | Fondations GDExtension + Flecs, Sérialisation SoA (`NetBatch`), Découplage `NetWorldCore`, Singleton & `ProjectSettings` | 🟢 Terminé (nettoyage final fait en `ae905f5`) |
| **[#01](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/01_2026-11-13_transport_multi_clients.md)** | `2026-11-13` | `3h 30m` | Transport UDP multi-clients (`peer_id`, broadcast, timeout, `NetEvent`) sur `network-logic` | 🟡 En cours (dispatcher / sync / spawn à faire) |
| **[#02](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/02_2026-11-14_stream_et_packet_dispatcher.md)** | `2026-11-14` | `7h 47m` | Migration C++20, `NetStream` (concepts), `StreamWriter`/`StreamReader` et `PacketDispatcher` | 🟡 En cours (câblage dans `NetWorldCore` à faire) |
| **[#03](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/03_2026-11-16_netmessage_et_sync_entites.md)** | `2026-11-16` | `4h 07m` | `NetMessage.h`, routage `PacketDispatcher` & `NetEvent` dans `NetWorldCore`, mapping d'IDs Flecs et signaux Godot | 🟡 En cours (keepalive client / despawn à stabiliser) |
| *#04* | *À venir* | *—* | *Copier `TEMPLATE_DEVLOG.md` vers `04_YYYY-MM-DD_titre.md`* | *—* |

---

## 💡 Comment utiliser ce Devlog au quotidien ?

1. **Avant de coder (2 min) :**
   - Ouvre ce [README.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/README.md) ou la section **« 5. Où j'en suis & Prochaine Session »** de ton dernier devlog pour te remettre immédiatement dans le bain.
2. **Pendant que tu codes :**
   - Duplique [TEMPLATE_DEVLOG.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/TEMPLATE_DEVLOG.md) en le nommant par exemple `01_2026-11-13_spawn_entites.md` et note ton heure de début.
3. **En fin de session (5-10 min) :**
   - Note l'heure de fin et le temps passé.
   - Remplis ce que tu as fait, **pourquoi** tu as fait ces choix techniques, et surtout la section **« 5. Où j'en suis & Prochaine Session »**.
   - Mets à jour le bloc **📌 Point de Reprise Rapide** et le tableau **🗂️ Historique des Sessions** dans ce `README.md`.
