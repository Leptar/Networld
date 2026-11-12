# Journal de Développement (Devlog) — Networld

Bienvenue dans le dossier **Devlog** de **Networld**. Ce dossier te permet de garder une trace claire de toute l'architecture du projet et de savoir exactement où tu t'es arrêté à chaque session de code.

---

## 📌 Point de Reprise Rapide (À mettre à jour en fin de session)

- **Dernière mise à jour :** `2026-11-12` (Commit `876342fb` — *REMOVE NetEntity*)
- **État actuel :** Réarchitecture `src/core/` vs `src/godot/` terminée, `NetWorld` passé en Singleton avec sélection du transport dans les `ProjectSettings`, et suppression de `NetEntity`.
- **À faire immédiatement à la reprise :**
  1. [ ] Retirer `#include "NetEntity.h"` et `ClassDB::register_class<NetEntity>();` oubliés dans [register_types.cpp](file:///c:/Users/leoul/Godot/Projects/Networld/src/godot/register_types.cpp).
  2. [ ] Dans [register_types.cpp](file:///c:/Users/leoul/Godot/Projects/Networld/src/godot/register_types.cpp), appeler `NetWorld::register_transport("Godot UDP", ...)` **avant** `memnew(NetWorld)` pour que le constructeur de `NetWorld` trouve bien le transport au démarrage.
  3. [ ] Dans [Networld.cpp](file:///c:/Users/leoul/Godot/Projects/Networld/src/godot/Networld.cpp), utiliser `create_transport()` dans `host()` et `join()` au lieu d'instancier `GodotUdpTransport` en dur.

---

## 📚 Index des Documents

| Fichier | Description |
| :--- | :--- |
| **[00_ARCHITECTURE_COMPLETE.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/00_ARCHITECTURE_COMPLETE.md)** | **Bilan complet du projet (2 Nov → 12 Nov 2026)** : Architecture `Core` vs `Godot`, explication détaillée de chaque fichier, schémas Mermaid (Flux, Sérialisation SoA, Boucle `step`), et choix techniques. |
| **[TEMPLATE_DEVLOG.md](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/TEMPLATE_DEVLOG.md)** | **Modèle de session** à copier-coller à chaque nouvelle session de code (`01_YYYY-MM-DD_sujet.md`, `02_...`). |

---

## 🗂️ Historique des Sessions

| Session | Date | Durée | Sujet principal | Statut |
| :--- | :--- | :--- | :--- | :--- |
| **[#00](file:///c:/Users/leoul/Godot/Projects/Networld/devlog/00_ARCHITECTURE_COMPLETE.md)** | `2026-11-02` → `2026-11-12` | ~10 jours | Fondations GDExtension + Flecs, Sérialisation SoA (`NetBatch`), Découplage `NetWorldCore`, Singleton & `ProjectSettings` | 🟡 Nettoyage post-suppression `NetEntity` à finir |
| *#01* | *À venir* | *—* | *Copier `TEMPLATE_DEVLOG.md` vers `01_YYYY-MM-DD_titre.md`* | *—* |

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
