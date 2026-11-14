# Journal de Développement (Devlog) — Networld

Bienvenue dans le dossier **Devlog** de **Networld**. Ce dossier te permet de garder une trace claire de toute l'architecture du projet et de savoir exactement où tu t'es arrêté à chaque session de code.

---

## 📌 Point de Reprise Rapide (À mettre à jour en fin de session)

- **Dernière mise à jour :** `2026-11-14` (Branche `network-logic`, Commit `40a3f83` — *ADD PacketDispatcher*)
- **État actuel :** Passage du projet en C++20 dans `SConstruct`, et implémentation complète des briques de sérialisation et de routage réseau dans `src/core/` : concepts C++20 (`NetStream.h`), `StreamWriter`, `StreamReader` (`std::span` / `std::optional`) et `PacketDispatcher` (`encode_packet` + table 256 handlers).
- **À faire immédiatement à la reprise :**
  1. [ ] Vérifier la compilation SCons complète après l'ajout de `PacketDispatcher`.
  2. [ ] Définir les messages réseau satisfaisant `NetMessage` (`knock`, `spawn`, `despawn`, `state_sync`).
  3. [ ] Dans [NetworldCore.cpp](file:///c:/Users/leoul/Godot/Projects/Networld/src/core/NetworldCore.cpp), brancher un `switch (pkt.event)` (`CONNECTED`, `DISCONNECTED`, `DATA`) dans `step()` et router `DATA` avec `PacketDispatcher`.
  4. [ ] Implémenter la logique de spawn/despawn et de synchronisation initiale lors de la connexion et déconnexion d'un client.

---

## 📚 Index des Documents

| Fichier | Description |
| :--- | :--- |
| **[00_ARCHITECTURE_COMPLETE.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/00_ARCHITECTURE_COMPLETE.md)** | **Bilan complet du projet (2 Nov → 12 Nov 2026)** : Architecture `Core` vs `Godot`, explication détaillée de chaque fichier, schémas Mermaid (Flux, Sérialisation SoA, Boucle `step`), et choix techniques. |
| **[01_2026-11-13_transport_multi_clients.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/01_2026-11-13_transport_multi_clients.md)** | **Session #01 (13 Nov 2026)** : Transport UDP multi-clients par `peer_id`, broadcast, timeouts et événements `NetEvent` sur `network-logic`. |
| **[02_2026-11-14_stream_et_packet_dispatcher.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/02_2026-11-14_stream_et_packet_dispatcher.md)** | **Session #02 (14 Nov 2026)** : Migration C++20 (`SConstruct`), concepts (`NetStream.h`), `StreamWriter`/`StreamReader` et `PacketDispatcher`. |
| **[TEMPLATE_DEVLOG.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/TEMPLATE_DEVLOG.md)** | **Modèle de session** à copier-coller à chaque nouvelle session de code (`01_YYYY-MM-DD_sujet.md`, `02_...`). |

---

## 🗂️ Historique des Sessions

| Session | Date | Durée | Sujet principal | Statut |
| :--- | :--- | :--- | :--- | :--- |
| **[#00](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/00_ARCHITECTURE_COMPLETE.md)** | `2026-11-02` → `2026-11-12` | ~10 jours | Fondations GDExtension + Flecs, Sérialisation SoA (`NetBatch`), Découplage `NetWorldCore`, Singleton & `ProjectSettings` | 🟢 Terminé (nettoyage final fait en `ae905f5`) |
| **[#01](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/01_2026-11-13_transport_multi_clients.md)** | `2026-11-13` | `3h 30m` | Transport UDP multi-clients (`peer_id`, broadcast, timeout, `NetEvent`) sur `network-logic` | 🟡 En cours (dispatcher / sync / spawn à faire) |
| **[#02](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/02_2026-11-14_stream_et_packet_dispatcher.md)** | `2026-11-14` | `7h 47m` | Migration C++20, `NetStream` (concepts), `StreamWriter`/`StreamReader` et `PacketDispatcher` | 🟡 En cours (câblage dans `NetWorldCore` à faire) |
| *#03* | *À venir* | *—* | *Copier `TEMPLATE_DEVLOG.md` vers `03_YYYY-MM-DD_titre.md`* | *—* |

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
