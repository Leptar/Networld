# Devlog #08 — Refonte `ServerRole` (1C) & `ClientRole` (1D)

| Métadonnées | Détails |
| :--- | :--- |
| **Date** | `2026-11-24` |
| **Heure (Début → Fin)** | `16:00` → `19:07` |
| **Temps passé** | `~3h 05m` |
| **Branche / Commit** | `step-1` (`47b0c11`) |
| **Statut de fin de session** | 🟡 WIP Étape 1 (1C terminée, 1D implémentée avec 3 correctifs à appliquer dès la reprise avant 1E → 1F) |

---

## 1. Contexte & Objectifs de la session
> *Où j'en étais en ouvrant le projet aujourd'hui, et qu'est-ce que je voulais accomplir pendant cette session ?*

À l'issue de la session #07 (`e1f49f1`), les contrats de données (**1A**) et la refonte de `NetWorldCore` (**1B**) étaient terminés, mais `ServerRole`, `ClientRole` et `NetWorld` contenaient encore des stubs temporaires (`TODO(1C/1D/1E)`). L'objectif du jour était de poursuivre l'Étape 1 en implémentant des rôles serveur et client 100 % découplés de tout gameplay.

- [x] Finaliser les micro-retouches **1B** sur `NetWorldCore` (`CollectDirectViews` + passage de `HostMode` à `ServerRole`) (`2a61073`)
- [x] Implémenter et valider la **sous-étape 1C** : refonte complète de `ServerRole` (Listen vs Dedicated, `WelcomeMsg`, file d'inputs anti-speedhack + vérif d'ownership, suppression de `ServerPlayer` et `MovementSim.h`) (`2a61073`)
- [x] Implémenter la **sous-étape 1D** : refonte de `ClientRole` (`WelcomeMsg` + garde-fou ordre UDP, mapping bidirectionnel `server_id ↔ local_id`, prédiction et replay générique via `run_entity_simulator`) (`47b0c11`)
- [ ] Appliquer les 3 correctifs repérés à la relecture de **1D** dans `ClientRole.cpp` (à faire en tout début de prochaine session)
- [ ] Sous-étapes **1E** (`NetWorld` + sortie de `MovementSim.h`) et **1F** (`demo/test_networld.gd` + tests complets) reportées à la prochaine session

---

## 2. Ce qui a été fait (Résumé des changements)
> *Liste claire des fonctionnalités ajoutées, modifiées ou supprimées.*

### Fichiers touchés
- `src/core/NetworldCore.cpp` (`2a61073`) :
  - Renommage du système `"CollectPredictedViewUpdates"` en `"CollectDirectViews"` (collecte les vues directes sans `InterpolationState` côté Serveur comme côté Client).
  - Passage de `mode` (`HostMode`) au constructeur `std::make_unique<ServerRole>(mode)` dans `start_host()`.
- `src/core/ServerRole.h` & `src/core/ServerRole.cpp` (`2a61073`) :
  - Suppression totale du gameplay de démo (`#include "MovementSim.h"`, `"ServerPlayer"`, `host_input_`, `host_accumulator_`, auto-spawn de joueur).
  - Support unifié `HostMode::DEDICATED` (`get_local_peer_id() == PEER_NONE`) et `HostMode::LISTEN` (`get_local_peer_id() == PEER_SERVER`, émission de `PeerEvent{PEER_SERVER, true}` dans `on_start`).
  - `on_peer_connected()` : enregistrement dans `connected_peers_`, envoi de `WelcomeMsg{peer_id, tick}`, synchronisation des entités déjà présentes (`SpawnEntityMsg`), puis `push_connected_peers({peer_id, false})`.
  - `on_entity_spawned()` / `on_entity_despawned()` : diffusion des `SpawnEntityMsg` / `DespawnEntityMsg` identiques à tous les `connected_peers_`.
  - `on_peer_disconnected()` : nettoyage des maps du pair et notification `push_disconnected_peers(peer_id)` sans détruire automatiquement ses entités (laissé à la couche jeu).
  - File d'inputs bornée (`MAX_PAYLOAD_INPUTS = 16`) séparant `last_received_seq_` (filtrage anti-doublon/hors-ordre à l'ingress) et `last_ack_seq_` (dernier `seq` réellement simulé, renvoyé dans `StateSyncMsg`).
  - `on_fixed_tick()` : consommation de 1 input max par client par tick avec vérification d'ownership (`NetOwnership::owner_peer_id == peer_id`) avant `run_entity_simulator()`, plus traitement de `host_local_inputs_` en mode `LISTEN`.
  - `on_stop()` : destruction générique de toutes les entités `.with<Networked>()` et purge complète des structures internes.
- `src/core/ClientRole.h` & `src/core/ClientRole.cpp` (`47b0c11`) :
  - Suppression de `#include "MovementSim.h"`, `input_client_`, `local_player_entity_id_` et `fixed_accumulator`.
  - Gestion de `WelcomeMsg` avec garde-fou d'ordre UDP (`deferred_spawns_` rejoués dès réception de `WelcomeMsg`).
  - Mapping bidirectionnel `net_to_local_entities_` (`server_id → local_id`) et `local_to_net_entities_` (`local_id → server_id`), avec ajout du composant `NetEntityID` sur les entités créées.
  - `handle_spawn_entity()` : attache `LocalPredicted` si `is_local && msg.predicted`, sinon `InterpolationState`.
  - `StateSyncMsg` : réconciliation générique rejouant `pending_inputs_` via `core.run_entity_simulator(..., SimContext{core, FIXED_DT, input.seq, true})` (`is_replay = true`).
  - `on_fixed_tick()` / `set_local_input()` : traduction `local_id → server_id`, envoi de `InputMsg{seq, server_id, payload}` et prédiction locale via `run_entity_simulator()`.
  - `on_egress()` simplifié au keep-alive `KnockMsg` (toutes les 30 ticks avant `WelcomeMsg`, puis toutes les 60 ticks).

### Explication du fonctionnement
- **Serveur neutre et sécurisé :** `ServerRole` ne sait plus ce qu'est un joueur ni comment il se déplace. Il valide l'appartenance (`NetOwnership`) et le rythme (1 input/tick depuis une file bornée) puis délègue le step au simulateur enregistré pour le `NetType::type_id` de l'entité.
- **Client générique avec réconciliation multi-entités :** `ClientRole` attend son `WelcomeMsg` pour connaître son `local_peer_id_`, traduit les IDs locaux en IDs serveur pour envoyer des `InputMsg` génériques, prédit localement chaque entité `LocalPredicted` et rejoue les inputs non acquittés avec `SimContext::is_replay = true`.

---

## 3. Choix Techniques & Architecture (Le "Pourquoi")
> *Pourquoi avoir codé ça de cette manière plutôt qu'une autre ? Quels compromis (performance, lisibilité, découplage) ont été faits ?*

- **Décision 1 : Conserver `peer_id` en `uint32_t` (et non `uint64_t`)**
  - *Question posée :* Passer `peer_id` en `uint64_t` par précaution contre le manque d'IDs ?
  - *Raison du choix :* `uint32_t` offre 4,29 milliards d'IDs par session serveur (> 13 ans à 10 connexions/s), reste moitié moins coûteux dans `NetOwnership` (SoA) et les paquets réseau, et ne risque jamais de déborder en négatif dans le `int` signé 64 bits de GDScript. À l'inverse, `entity_id` reste en `uint64_t` car c'est le type natif de `flecs::entity_t` (index + génération + flags). Les IDs de compte persistants (SteamID64, UUID) iront dans `NetPlayerState` en Couche 3.
- **Décision 2 : Séparer `last_received_seq_` et `last_ack_seq_` sur `ServerRole`**
  - *Raison du choix :* Un input reçu en rafale est mis en file d'attente et ne sera simulé qu'au tick suivant. Acquitter dès la réception fausserait `StateSyncMsg::ack_input_seq` et casserait la réconciliation client. `last_ack_seq_` n'avance donc qu'une fois l'input réellement consommé dans `on_fixed_tick()`.
- **Décision 3 : Garde-fou `deferred_spawns_` côté `ClientRole`**
  - *Raison du choix :* En UDP brut (`GodotUdpTransport`), un `SpawnEntityMsg` peut arriver avant `WelcomeMsg`. Mettre ces spawns en attente jusqu'à connaître `local_peer_id_` garantit de toujours classifier correctement `is_local` (`LocalPredicted` vs `InterpolationState`).

---

## 4. Problèmes rencontrés & Solutions (Debugging)
> *Qu'est-ce qui a posé problème ou fait perdre du temps aujourd'hui, et comment ça a été résolu ?*

- **Problème 1 : `return;` au lieu de `continue;` dans `ServerRole::on_fixed_tick()` (`2a61073`)**
  - *Cause :* `if (queue.empty()) return;` sortait de toute la fonction dès qu'un client avait une file vide, bloquant les autres clients et le Host Listen-Server.
  - *Solution :* Corrigé en `continue;` après relecture.
- **Problème 2 : Clé `uint32_t` au lieu de `uint64_t` sur `ServerRole::host_local_inputs_` (`2a61073`)**
  - *Cause :* Un `flecs::entity_t` est un `uint64_t` (bits hauts = génération/flags) ; une clé `uint32_t` tronquait l'ID.
  - *Solution :* Passé en `std::unordered_map<uint64_t, InputPayload>` avant commit.
- **Problème 3 : 3 bugs repérés à la relecture de `ClientRole.cpp` (`47b0c11`, à corriger à la reprise)**
  - `on_fixed_tick()` L187 : condition inversée `if (welcome_received_)` au lieu de `if (!welcome_received_)`, et `staged_local_inputs_.clear();` manquant en fin de fonction.
  - `DespawnEntityMsg` L100-108 : `uint64_t& to_despawn_id = it->second;` crée une référence pendante (*use-after-free*) après `net_to_local_entities_.erase(it)` juste avant `core.destroy_entity(to_despawn_id)` → retirer le `&`.
  - `handle_spawn_entity()` L32-38 : garde anti-doublon `if (net_to_local_entities_.contains(msg.entity_Id)) return;` et `msg.rotation.normalized()` manquants.

---

## 5. Où j'en suis & Prochaine Session (Reprise rapide)
> *LA section à relire en premier la prochaine fois que j'ouvre le projet !*

- **État exact du code en quittant :**
  - Branche `step-1`, commit `47b0c11` propre (arborescence sans modifications non commitées).
  - **1A**, **1B** et **1C (`ServerRole`)** sont **terminées et relues**.
  - **1D (`ClientRole`)** est codée et commitée, mais nécessite les **3 correctifs rapides** listés ci-dessous avant de tester.
- **Prochaines étapes immédiates (TODO de reprise) :**
  1. [ ] **Correctifs 1D (`src/core/ClientRole.cpp` & `ClientRole.h`) :**
     - L187 : changer `if (welcome_received_)` en `if (!welcome_received_)` et ajouter `staged_local_inputs_.clear();` à la fin de `on_fixed_tick()`.
     - L100 : changer `uint64_t& to_despawn_id` en `const uint64_t to_despawn_id` (corrige le *use-after-free*).
     - L32 : ajouter `if (net_to_local_entities_.contains(msg.entity_Id)) return;` et normaliser `msg.rotation.normalized()` dans `handle_spawn_entity()`.
     - Retirer `#include <set>` inutilisé dans `ClientRole.h`.
  2. [ ] **Sous-étape 1E (`src/godot/Networld.h/.cpp` + sortir `MovementSim.h`) :**
     - Déplacer `src/core/MovementSim.h` → `src/godot/DemoMovementSim.h` (`src/core/` devient 100 % pur, 0 gameplay).
     - Enregistrer le simulateur `type_id = 1` dans `NetWorld::NetWorld()`.
     - Exposer `host_listen`, `host_dedicated`, `stop_session`, `get_local_peer_id`, `spawn_entity`, `despawn_entity`, `destroy_entities_owned_by`, `set_local_move_input`.
     - Séparer `_physics_process` (`step_simulation` + copie/vidage avant émission des 4 signaux `peer_connected`, `peer_disconnected`, `entity_despawned`, `entity_spawned` avec ses 6 arguments) et `_process` (`step_visual` + synchro `Node3D`).
     - Compiler avec `scons target=template_debug`.
  3. [ ] **Sous-étape 1F (`demo/test_networld.gd` + validation) :**
     - 3 boutons (**Dedicated Server**, **Listen-Server**, **Client**), spawn piloté par `peer_connected` / `peer_disconnected`, tests multi-instances + vérification Flecs Explorer.
  4. [ ] **Préparation Étape 2 :**
     - Concevoir le contrat `INetPhysics` + `DefaultKinematicPhysics` (`src/core/`) et `GodotPhysicsBridge` (`PhysicsServer3D::body_test_motion` dans `src/godot/`).
