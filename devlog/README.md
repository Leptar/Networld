# Journal de Développement (Devlog) — Networld

Bienvenue dans le dossier **Devlog** de **Networld**. Ce dossier te permet de garder une trace claire de toute l'architecture du projet et de savoir exactement où tu t'es arrêté à chaque session de code.

---

## 📌 Point de Reprise Rapide (À mettre à jour en fin de session)

- **Dernière mise à jour :** `2026-11-18` (Branche `network-logic`, Commit `786fdd2` — *UPDATE NetworldCore*)
- **État actuel :** Propriété locale du joueur implémentée de bout en bout (`SpawnEntityMsg::is_owner`, `EntitySpawnEntry::is_local`, signal `entity_spawned(id, pos, is_local)`, coloration du cube local et suivi caméra sécurisé dans [test_networld.gd](file:///c:/Users/leoul/Godot/Projects/Networld/demo/test_networld.gd)), et séparation complète des rôles Serveur/Client via [INetRole.h](file:///c:/Users/leoul/Godot/Projects/Networld/src/core/INetRole.h), [ServerRole.cpp](file:///c:/Users/leoul/Godot/Projects/Networld/src/core/ServerRole.cpp) et [ClientRole.cpp](file:///c:/Users/leoul/Godot/Projects/Networld/src/core/ClientRole.cpp) avec désactivation des systèmes de simulation Flecs côté client (`set_simulation_systems_enabled(false)`).
- **À faire immédiatement à la reprise :**
  1. [ ] Mettre en place l'interpolation visuelle côté client (`ClientRole`) entre deux `StateSyncMsg` (puis prédiction/réconciliation si besoin).
  2. [ ] Mini-nettoyage post-split : ajouter `#include <cmath>` dans [ServerRole.cpp](file:///c:/Users/leoul/Godot/Projects/Networld/src/core/ServerRole.cpp), retirer les includes inutilisés de [NetworldCore.h](file:///c:/Users/leoul/Godot/Projects/Networld/src/core/NetworldCore.h) / [NetworldCore.cpp](file:///c:/Users/leoul/Godot/Projects/Networld/src/core/NetworldCore.cpp) et sortir la vitesse serveur (`5.0f`) en constante/composant propre.
  3. [ ] Ajouter la synchronisation de rotation (`SpawnEntityMsg::rotation`) et décider du contrôle local sur le Host (Listen-Server).
  4. [ ] Concevoir la couche de fiabilité (`RELIABLE_EVENTS` : ACK / resend) pour `SpawnEntityMsg` / `DespawnEntityMsg`.

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
| *#06* | *À venir* | *—* | *Copier `TEMPLATE_DEVLOG.md` vers `06_YYYY-MM-DD_titre.md`* | *—* |

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
