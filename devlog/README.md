# Journal de Développement (Devlog) — Networld

Bienvenue dans le dossier **Devlog** de **Networld**. Ce dossier te permet de garder une trace claire de toute l'architecture du projet et de savoir exactement où tu t'es arrêté à chaque session de code.

---

## 📌 Point de Reprise Rapide (À mettre à jour en fin de session)

- **Dernière mise à jour :** `2026-11-21` (Branche `step-1`, Commit `e1f49f1` — *add comment*)
- **État actuel :** Plan directeur v0.1.0 figé dans [ARCHITECTURE_V0_1.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/ARCHITECTURE_V0_1.md) (écosystème 3 repos : `Networld`, `GodotNetworld`, `QuicNetworkGodot` + 4 contrats validés). **Sous-étapes 1A & 1B terminées** (`InputPayload`, `WelcomeMsg`, `NetOwnership`, `HostMode`, `NetWorldCore` sans gameplay avec 2 pipelines Flecs `step_simulation`/`step_visual` et registre de simulateurs). Build `template_debug` vert grâce aux stubs temporaires (`TODO(1C/1D/1E)`) sur `ServerRole`, `ClientRole` et `NetWorld`.
- **À faire immédiatement à la reprise (suite de l'Étape 1) :**
  1. [ ] **Sous-étape 1C (`ServerRole`) :** `HostMode` (`DEDICATED`/`LISTEN`), purger `ServerPlayer` & `MovementSim.h`, `WelcomeMsg`, broadcast `SpawnEntityMsg`/`DespawnEntityMsg`, et file d'inputs bornée anti-speedhack + vérif d'ownership dans `on_fixed_tick()`.
  2. [ ] **Sous-étape 1D (`ClientRole`) :** `WelcomeMsg` + garde-fou UDP, mapping `server_id ↔ local_id`, `on_fixed_tick()` + replay via `run_entity_simulator(..., is_replay=true)`.
  3. [ ] **Sous-étape 1E (`Networld.h/.cpp`) :** sortir `MovementSim.h` de `src/core/`, séparer `_physics_process`/`_process`, exposer `host_listen`/`host_dedicated`, `spawn_entity`, `destroy_entities_owned_by`, signaux `peer_connected`/`peer_disconnected` et aligner `entity_spawned`.
  4. [ ] **Sous-étape 1F (`demo/test_networld.gd`) :** 3 boutons (DGS / Listen-Server / Client), spawn piloté par `peer_connected`, tests complets + Flecs Explorer.

---

## 📚 Index des Documents

| Fichier | Description |
| :--- | :--- |
| **[00_ARCHITECTURE_COMPLETE.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/00_ARCHITECTURE_COMPLETE.md)** | **Bilan complet du projet (2 Nov → 12 Nov 2026)** : Architecture `Core` vs `Godot`, explication détaillée de chaque fichier, schémas Mermaid (Flux, Sérialisation SoA, Boucle `step`), et choix techniques. |
| **[01_2026-11-13_transport_multi_clients.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/01_2026-11-13_transport_multi_clients.md)** | **Session #01 (13 Nov 2026)** : Transport UDP multi-clients par `peer_id`, broadcast, timeouts et événements `NetEvent` sur `network-logic`. |
| **[02_2026-11-14_stream_et_packet_dispatcher.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/02_2026-11-14_stream_et_packet_dispatcher.md)** | **Session #02 (14 Nov 2026)** : Migration C++20 (`SConstruct`), concepts (`NetStream.h`), `StreamWriter`/`StreamReader` et `PacketDispatcher`. |
| **[03_2026-11-16_netmessage_et_sync_entites.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/03_2026-11-16_netmessage_et_sync_entites.md)** | **Session #03 (16 Nov 2026)** : Messages `NetMessage.h`, câblage `PacketDispatcher`/`NetEvent` dans `NetWorldCore`, mapping d'IDs Flecs et signaux Godot (`entity_spawned`/`entity_despawned`). |
| **[04_2026-11-17_keepalive_nettoyage_et_inputs.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/04_2026-11-17_keepalive_nettoyage_et_inputs.md)** | **Session #04 (17 Nov 2026)** : Keepalive UDP (`KnockMsg`), nettoyage technique `Core`/`Godot`, et inputs joueur autoritaires (`InputMsg`) jusqu'à GDScript. |
| **[05_2026-11-18_propriete_locale_et_separation_roles.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/05_2026-11-18_propriete_locale_et_separation_roles.md)** | **Session #05 (18 Nov 2026)** : Propriété locale (`is_owner` / `is_local` + caméra), interface `INetRole`, extraction `ServerRole`/`ClientRole` et séparation de la simulation Flecs. |
| **[06_2026-11-19_interpolation_rotation_et_prediction.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/06_2026-11-19_interpolation_rotation_et_prediction.md)** | **Session #06 (19 Nov 2026)** : Cadence sync 20 Hz, interpolation client (`InterpolationState`), rotation en quaternions (`Quat`/`nlerp`), prédiction & réconciliation (`MovementSim.h`, `ack_input_seq`) et mode Host (Listen-Server). |
| **[07_2026-11-21_architecture_v0_1_et_core_generique.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/07_2026-11-21_architecture_v0_1_et_core_generique.md)** | **Session #07 (21 Nov 2026)** : Architecture v0.1.0 (3 repos, 3 couches, 4 contrats), sous-étapes 1A (`InputPayload`, `WelcomeMsg`, `NetOwnership`, `HostMode`) & 1B (`NetWorldCore` sans gameplay, 2 pipelines Flecs, registre de simulateurs) et stubs temporaires compilables. |
| **[TEMPLATE_DEVLOG.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/TEMPLATE_DEVLOG.md)** | **Modèle de session** à copier-coller à chaque nouvelle session de code (`01_YYYY-MM-DD_sujet.md`, `02_...`). |

---

## 🗂️ Historique des Sessions

| Session | Date | Durée | Sujet principal | Statut |
| :--- | :--- | :--- | :--- | :--- |
| **[#00](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/00_ARCHITECTURE_COMPLETE.md)** | `2026-11-02` → `2026-11-12` | ~10 jours | Fondations GDExtension + Flecs, Sérialisation SoA (`NetBatch`), Découplage `NetWorldCore`, Singleton & `ProjectSettings` | 🟢 Terminé (nettoyage final fait en `ae905f5`) |
| **[#01](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/01_2026-11-13_transport_multi_clients.md)** | `2026-11-13` | `3h 30m` | Transport UDP multi-clients (`peer_id`, broadcast, timeout, `NetEvent`) sur `network-logic` | 🟡 En cours (dispatcher / sync / spawn à faire) |
| **[#02](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/02_2026-11-14_stream_et_packet_dispatcher.md)** | `2026-11-14` | `7h 47m` | Migration C++20, `NetStream` (concepts), `StreamWriter`/`StreamReader` et `PacketDispatcher` | 🟡 En cours (câblage dans `NetWorldCore` à faire) |
| **[#03](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/03_2026-11-16_netmessage_et_sync_entites.md)** | `2026-11-16` | `4h 07m` | `NetMessage.h`, routage `PacketDispatcher` & `NetEvent` dans `NetWorldCore`, mapping d'IDs Flecs et signaux Godot | 🟡 En cours (keepalive client / despawn à stabiliser) |
| **[#04](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/04_2026-11-17_keepalive_nettoyage_et_inputs.md)** | `2026-11-17` | `5h 24m` | Keepalive UDP, nettoyage Core/Godot, et inputs joueur autoritaires (`InputMsg`) jusqu'à GDScript | 🟢 Terminé (commit `c6e3a33`) |
| **[#05](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/05_2026-11-18_propriete_locale_et_separation_roles.md)** | `2026-11-18` | `~6h 25m` | Propriété locale (`is_local`, suivi caméra) et séparation `INetRole` (`ServerRole` / `ClientRole`) | 🟢 Terminé (commit `786fdd2`) |
| **[#06](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/06_2026-11-19_interpolation_rotation_et_prediction.md)** | `2026-11-19` | `~7h 30m` | Sync 20 Hz, interpolation client, rotation (`Quat`), prédiction/réconciliation (`MovementSim.h`) & Host jouable | 🟢 Terminé (commit `fea16ea`) |
| **[#07](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/07_2026-11-21_architecture_v0_1_et_core_generique.md)** | `2026-11-21` | `~7h 55m` | Architecture v0.1.0, contrats 1A (`InputPayload`, `WelcomeMsg`, `NetOwnership`) & refonte `NetWorldCore` 1B (2 pipelines Flecs) | 🟡 Compilable (WIP `step-1`, commit `e1f49f1` — 1C → 1F à venir) |
| *#08* | *À venir* | *—* | *Copier `TEMPLATE_DEVLOG.md` vers `08_YYYY-MM-DD_titre.md`* | *—* |

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
