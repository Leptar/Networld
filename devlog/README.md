# Journal de Développement (Devlog) — Networld

Bienvenue dans le dossier **Devlog** de **Networld**. Ce dossier te permet de garder une trace claire de toute l'architecture du projet et de savoir exactement où tu t'es arrêté à chaque session de code.

---

## 📌 Point de Reprise Rapide (À mettre à jour en fin de session)

- **Dernière mise à jour :** `2026-11-13` (Branche `network-logic`, Commit `65dbcf3` — *UPDATE Transport*)
- **État actuel :** Nettoyage `register_types.cpp` / `NetWorld` terminé (`ae905f5`), et `GodotUdpTransport` gère désormais le multi-clients par `peer_id` virtuel, le broadcast (`PEER_BROADCAST`), les événements `NetEvent` (`CONNECTED`/`DISCONNECTED`/`DATA`) et le timeout (`5000 ms`).
- **À faire immédiatement à la reprise :**
  1. [ ] Dans [NetworldCore.cpp](file:///c:/Users/leoul/Godot/Projects/Networld/src/core/NetworldCore.cpp), brancher un switch/dispatch sur `pkt.event` (`CONNECTED`, `DISCONNECTED`, `DATA`) dans `step()`.
  2. [ ] Définir un en-tête / type de message pour distinguer `knock`, `spawn`, `despawn` et `state_sync` (batches SoA) dans `NetPacket::payload`.
  3. [ ] Implémenter la logique de spawn/despawn et de synchronisation initiale lors de la connexion et déconnexion d'un client.

---

## 📚 Index des Documents

| Fichier | Description |
| :--- | :--- |
| **[00_ARCHITECTURE_COMPLETE.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/00_ARCHITECTURE_COMPLETE.md)** | **Bilan complet du projet (2 Nov → 12 Nov 2026)** : Architecture `Core` vs `Godot`, explication détaillée de chaque fichier, schémas Mermaid (Flux, Sérialisation SoA, Boucle `step`), et choix techniques. |
| **[01_2026-11-13_transport_multi_clients.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/01_2026-11-13_transport_multi_clients.md)** | **Session #01 (13 Nov 2026)** : Transport UDP multi-clients par `peer_id`, broadcast, timeouts et événements `NetEvent` sur `network-logic`. |
| **[TEMPLATE_DEVLOG.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/TEMPLATE_DEVLOG.md)** | **Modèle de session** à copier-coller à chaque nouvelle session de code (`01_YYYY-MM-DD_sujet.md`, `02_...`). |

---

## 🗂️ Historique des Sessions

| Session | Date | Durée | Sujet principal | Statut |
| :--- | :--- | :--- | :--- | :--- |
| **[#00](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/00_ARCHITECTURE_COMPLETE.md)** | `2026-11-02` → `2026-11-12` | ~10 jours | Fondations GDExtension + Flecs, Sérialisation SoA (`NetBatch`), Découplage `NetWorldCore`, Singleton & `ProjectSettings` | 🟢 Terminé (nettoyage final fait en `ae905f5`) |
| **[#01](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/01_2026-11-13_transport_multi_clients.md)** | `2026-11-13` | `3h 30m` | Transport UDP multi-clients (`peer_id`, broadcast, timeout, `NetEvent`) sur `network-logic` | 🟡 En cours (dispatcher / sync / spawn à faire) |
| *#02* | *À venir* | *—* | *Copier `TEMPLATE_DEVLOG.md` vers `02_YYYY-MM-DD_titre.md`* | *—* |

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
