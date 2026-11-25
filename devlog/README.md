# Journal de Développement (Devlog) — Networld

Bienvenue dans le dossier **Devlog** de **Networld**. Ce dossier te permet de garder une trace claire de toute l'architecture du projet et de savoir exactement où tu t'es arrêté à chaque session de code.

---

## 📌 Point de Reprise Rapide (À mettre à jour en fin de session)

- **Dernière mise à jour :** `2026-11-24` (Branche `step-1`, Commit `47b0c11` — *UPDATE ClientRole*)
- **État actuel :** Étape 1 (découplage Core v0.1.0) bien avancée : **1A** (contrats de données), **1B** (`NetWorldCore` + 2 pipelines Flecs) et **1C** (`ServerRole` Listen/Dedicated, file anti-speedhack + ownership) sont terminées ; **1D** (`ClientRole` générique avec `WelcomeMsg`, garde-fou UDP, mapping bidirectionnel et réconciliation générique) est codée avec 3 petits correctifs à appliquer dès la reprise avant d'enchaîner sur **1E** et **1F**.
- **À faire immédiatement à la reprise :**
  1. [ ] Appliquer les 3 correctifs de relecture dans [ClientRole.cpp](../src/core/ClientRole.cpp) (`if (!welcome_received_)` + `staged_local_inputs_.clear()` L187-214, `const uint64_t to_despawn_id` sans `&` L100, garde anti-doublon + `rotation.normalized()` dans `handle_spawn_entity` L32-38) et retirer `#include <set>` dans [ClientRole.h](../src/core/ClientRole.h).
  2. [ ] **Sous-étape 1E :** déplacer [MovementSim.h](../src/core/MovementSim.h) hors de `src/core/` (`src/godot/DemoMovementSim.h`), enregistrer le simulateur `type_id = 1` dans [Networld.cpp](../src/godot/Networld.cpp), exposer la nouvelle API (`host_listen`/`host_dedicated`, `spawn_entity`, `despawn_entity`, `destroy_entities_owned_by`, `set_local_move_input`) et séparer `_physics_process` (`step_simulation` + 4 signaux) et `_process` (`step_visual` + `Node3D`).
  3. [ ] **Sous-étape 1F :** mettre à jour [test_networld.gd](../demo/test_networld.gd) (3 boutons Dedicated / Listen / Client + spawn sur `peer_connected`) et valider multi-instances + Flecs Explorer.
  4. [ ] **Préparation Étape 2 :** concevoir `INetPhysics` + `DefaultKinematicPhysics` (`src/core/`) et `GodotPhysicsBridge` (`PhysicsServer3D::body_test_motion` dans `src/godot/`).

---

## 📚 Index des Documents

| Fichier | Description |
| :--- | :--- |
| **[00_ARCHITECTURE_COMPLETE.md](./00_ARCHITECTURE_COMPLETE.md)** | **Bilan complet du projet (2 Nov → 12 Nov 2026)** : Architecture `Core` vs `Godot`, explication détaillée de chaque fichier, schémas Mermaid (Flux, Sérialisation SoA, Boucle `step`), et choix techniques. |
| **[ARCHITECTURE_V0_1.md](./ARCHITECTURE_V0_1.md)** | **Plan Directeur v0.1.0 (21 Nov 2026)** : Écosystème 3 dépôts (`Networld`, `GodotNetworld`, `QuicNetworkGodot`), architecture 3 couches, QUIC, `INetPhysics`, feuille de route en 4 étapes et 4 contrats validés de l'Étape 1. |
| **[01_2026-11-13_transport_multi_clients.md](./01_2026-11-13_transport_multi_clients.md)** | **Session #01 (13 Nov 2026)** : Transport UDP multi-clients par `peer_id`, broadcast, timeouts et événements `NetEvent` sur `network-logic`. |
| **[02_2026-11-14_stream_et_packet_dispatcher.md](./02_2026-11-14_stream_et_packet_dispatcher.md)** | **Session #02 (14 Nov 2026)** : Migration C++20 (`SConstruct`), concepts (`NetStream.h`), `StreamWriter`/`StreamReader` et `PacketDispatcher`. |
| **[03_2026-11-16_netmessage_et_sync_entites.md](./03_2026-11-16_netmessage_et_sync_entites.md)** | **Session #03 (16 Nov 2026)** : Messages `NetMessage.h`, câblage `PacketDispatcher`/`NetEvent` dans `NetWorldCore`, mapping d'IDs Flecs et signaux Godot (`entity_spawned`/`entity_despawned`). |
| **[04_2026-11-17_keepalive_nettoyage_et_inputs.md](./04_2026-11-17_keepalive_nettoyage_et_inputs.md)** | **Session #04 (17 Nov 2026)** : Keepalive UDP (`KnockMsg`), nettoyage technique `Core`/`Godot`, et inputs joueur autoritaires (`InputMsg`) jusqu'à GDScript. |
| **[05_2026-11-18_propriete_locale_et_separation_roles.md](./05_2026-11-18_propriete_locale_et_separation_roles.md)** | **Session #05 (18 Nov 2026)** : Propriété locale (`is_owner` / `is_local` + caméra), interface `INetRole`, extraction `ServerRole`/`ClientRole` et séparation de la simulation Flecs. |
| **[06_2026-11-19_interpolation_rotation_et_prediction.md](./06_2026-11-19_interpolation_rotation_et_prediction.md)** | **Session #06 (19 Nov 2026)** : Cadence sync 20 Hz, interpolation client (`InterpolationState`), rotation en quaternions (`Quat`/`nlerp`), prédiction & réconciliation (`MovementSim.h`, `ack_input_seq`) et mode Host (Listen-Server). |
| **[07_2026-11-21_architecture_v0_1_et_core_generique.md](./07_2026-11-21_architecture_v0_1_et_core_generique.md)** | **Session #07 (21 Nov 2026)** : Architecture v0.1.0, contrats 1A (`Peer`/`Ownership`, `WelcomeMsg`, `InputPayload`, `INetRole`) et refonte `NetWorldCore` 1B (2 pipelines Flecs, registre de simulateurs). |
| **[08_2026-11-24_server_role_et_client_role.md](./08_2026-11-24_server_role_et_client_role.md)** | **Session #08 (24 Nov 2026)** : Refonte `ServerRole` 1C (Dedicated/Listen, file anti-speedhack + ownership) et `ClientRole` 1D (`WelcomeMsg`, garde-fou UDP, mapping bidirectionnel, replay générique). |
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
| **[#07](./07_2026-11-21_architecture_v0_1_et_core_generique.md)** | `2026-11-21` | `~7h 55m` | Plan directeur v0.1.0, contrats 1A & refonte `NetWorldCore` 1B (2 pipelines Flecs, registre de simulateurs) | 🟡 WIP Étape 1 (commit `e1f49f1`) |
| **[#08](./08_2026-11-24_server_role_et_client_role.md)** | `2026-11-24` | `~3h 05m` | Refonte `ServerRole` (1C) & `ClientRole` (1D) découplés de tout gameplay | 🟡 WIP Étape 1 (commit `47b0c11`, 3 fixes 1D + 1E/1F à suivre) |
| *#09* | *À venir* | *—* | *Copier `TEMPLATE_DEVLOG.md` vers `09_YYYY-MM-DD_titre.md`* | *—* |

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
