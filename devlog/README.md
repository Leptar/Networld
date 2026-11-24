# Journal de Développement (Devlog) — Networld

Bienvenue dans le dossier **Devlog** de **Networld**. Ce dossier te permet de garder une trace claire de toute l'architecture du projet et de savoir exactement où tu t'es arrêté à chaque session de code.

---

## 📌 Point de Reprise Rapide (À mettre à jour en fin de session)

- **Dernière mise à jour :** `2026-11-19` (Branche `network-logic`, Commit `fea16ea` — *UPDATE ServerRole*)
- **État actuel :** Pipeline réseau temps réel complet en place : synchronisation serveur cadencée à 20 Hz (`NET_SYNC_INTERVAL`), interpolation position + rotation (`Quat` / `nlerp`) pour les entités distantes (`InterpolationState`, `InterpolateViews`), prédiction à pas fixe 60 Hz + réconciliation serveur (`MovementSim.h`, `LocalPredicted`, `ack_input_seq`, `pending_inputs`) pour le joueur local, et mode Listen-Server (Host jouable via `ServerRole::set_client_input()`).
- **À faire immédiatement à la reprise :**
  1. [ ] Corriger deux petits détails : `if (pos || rot)` → `if (pos && rot)` dans [ClientRole.cpp](../src/core/ClientRole.cpp#L52), et aligner les 4 arguments du signal `entity_spawned(entity_id, position, rotation, is_local)` entre [Networld.cpp](../src/godot/Networld.cpp#L58-L62) (`emit_signal` L165) et [test_networld.gd](../demo/test_networld.gd#L47).
  2. [ ] Trancher le mode **Serveur dédié vs Listen-Server (Host)** (séparation éventuelle des modes) et nettoyer les blocs commentés dans [ServerRole.cpp](../src/core/ServerRole.cpp) et [ClientRole.cpp](../src/core/ClientRole.cpp).
  3. [ ] Harmoniser les systèmes Flecs (`ApplyGravity` / `ApplyMovement`) avec `simulate_player_step()` ([MovementSim.h](../src/core/MovementSim.h)).
  4. [ ] Concevoir la couche de fiabilité (`RELIABLE_EVENTS` : ACK / resend) pour `SpawnEntityMsg` / `DespawnEntityMsg`.

---

## 📚 Index des Documents

| Fichier | Description |
| :--- | :--- |
| **[00_ARCHITECTURE_COMPLETE.md](./00_ARCHITECTURE_COMPLETE.md)** | **Bilan complet du projet (2 Nov → 12 Nov 2026)** : Architecture `Core` vs `Godot`, explication détaillée de chaque fichier, schémas Mermaid (Flux, Sérialisation SoA, Boucle `step`), et choix techniques. |
| **[01_2026-11-13_transport_multi_clients.md](./01_2026-11-13_transport_multi_clients.md)** | **Session #01 (13 Nov 2026)** : Transport UDP multi-clients par `peer_id`, broadcast, timeouts et événements `NetEvent` sur `network-logic`. |
| **[02_2026-11-14_stream_et_packet_dispatcher.md](./02_2026-11-14_stream_et_packet_dispatcher.md)** | **Session #02 (14 Nov 2026)** : Migration C++20 (`SConstruct`), concepts (`NetStream.h`), `StreamWriter`/`StreamReader` et `PacketDispatcher`. |
| **[03_2026-11-16_netmessage_et_sync_entites.md](./03_2026-11-16_netmessage_et_sync_entites.md)** | **Session #03 (16 Nov 2026)** : Messages `NetMessage.h`, câblage `PacketDispatcher`/`NetEvent` dans `NetWorldCore`, mapping d'IDs Flecs et signaux Godot (`entity_spawned`/`entity_despawned`). |
| **[04_2026-11-17_keepalive_nettoyage_et_inputs.md](./04_2026-11-17_keepalive_nettoyage_et_inputs.md)** | **Session #04 (17 Nov 2026)** : Keepalive UDP (`KnockMsg`), nettoyage technique `Core`/`Godot`, et inputs joueur autoritaires (`InputMsg`) jusqu'à GDScript. |
| **[05_2026-11-18_propriete_locale_et_separation_roles.md](./05_2026-11-18_propriete_locale_et_separation_roles.md)** | **Session #05 (18 Nov 2026)** : Propriété locale (`is_owner` / `is_local` + caméra), interface `INetRole`, extraction `ServerRole`/`ClientRole` et séparation de la simulation Flecs. |
| **[06_2026-11-19_interpolation_rotation_et_prediction.md](./06_2026-11-19_interpolation_rotation_et_prediction.md)** | **Session #06 (19 Nov 2026)** : Cadence sync 20 Hz, interpolation client (`InterpolationState`), rotation en quaternions (`Quat`/`nlerp`), prédiction & réconciliation (`MovementSim.h`, `ack_input_seq`) et mode Host (Listen-Server). |
| **[TEMPLATE_DEVLOG.md](./TEMPLATE_DEVLOG.md)** | **Modèle de session** à copier-coller à chaque nouvelle session de code (`01_YYYY-MM-DD_sujet.md`, `02_...`). |

---

## 🗂️ Historique des Sessions

| Session | Date | Durée | Sujet principal | Statut |
| :--- | :--- | :--- | :--- | :--- |
| **[#00](./00_ARCHITECTURE_COMPLETE.md)** | `2026-11-02` → `2026-11-12` | ~10 jours | Fondations GDExtension + Flecs, Sérialisation SoA (`NetBatch`), Découplage `NetWorldCore`, Singleton & `ProjectSettings` | 🟢 Terminé (nettoyage final fait en `ae905f5`) |
| **[#01](./01_2026-11-13_transport_multi_clients.md)** | `2026-11-13` | `3h 30m` | Transport UDP multi-clients (`peer_id`, broadcast, timeout, `NetEvent`) sur `network-logic` | 🟡 En cours (dispatcher / sync / spawn à faire) |
| **[#02](./02_2026-11-14_stream_et_packet_dispatcher.md)** | `2026-11-14` | `7h 47m` | Migration C++20, `NetStream` (concepts), `StreamWriter`/`StreamReader` et `PacketDispatcher` | 🟡 En cours (câblage dans `NetWorldCore` à faire) |
| **[#03](./03_2026-11-16_netmessage_et_sync_entites.md)** | `2026-11-16` | `4h 07m` | `NetMessage.h`, routage `PacketDispatcher` & `NetEvent` dans `NetWorldCore`, mapping d'IDs Flecs et signaux Godot | 🟡 En cours (keepalive client / despawn à stabiliser) |
| **[#04](./04_2026-11-17_keepalive_nettoyage_et_inputs.md)** | `2026-11-17` | `5h 24m` | Keepalive UDP, nettoyage Core/Godot, et inputs joueur autoritaires (`InputMsg`) jusqu'à GDScript | 🟢 Terminé (commit `c6e3a33`) |
| **[#05](./05_2026-11-18_propriete_locale_et_separation_roles.md)** | `2026-11-18` | `~6h 25m` | Propriété locale (`is_local`, suivi caméra) et séparation `INetRole` (`ServerRole` / `ClientRole`) | 🟢 Terminé (commit `786fdd2`) |
| **[#06](./06_2026-11-19_interpolation_rotation_et_prediction.md)** | `2026-11-19` | `~7h 30m` | Sync 20 Hz, interpolation client, rotation (`Quat`), prédiction/réconciliation (`MovementSim.h`) & Host jouable | 🟢 Terminé (commit `fea16ea`) |
| *#07* | *À venir* | *—* | *Copier `TEMPLATE_DEVLOG.md` vers `07_YYYY-MM-DD_titre.md`* | *—* |

---

## 💡 Comment utiliser ce Devlog au quotidien ?

1. **Avant de coder (2 min) :**
   - Ouvre ce [README.md](./README.md) ou la section **« 5. Où j'en suis & Prochaine Session »** de ton dernier devlog pour te remettre immédiatement dans le bain.
2. **Pendant que tu codes :**
   - Duplique [TEMPLATE_DEVLOG.md](./TEMPLATE_DEVLOG.md) en le nommant par exemple `01_2026-11-13_spawn_entites.md` et note ton heure de début.
3. **En fin de session (5-10 min) :**
   - Note l'heure de fin et le temps passé.
   - Remplis ce que tu as fait, **pourquoi** tu as fait ces choix techniques, et surtout la section **« 5. Où j'en suis & Prochaine Session »**.
   - Mets à jour le bloc **📌 Point de Reprise Rapide** et le tableau **🗂️ Historique des Sessions** dans ce `README.md`.
