# Devlog #02 — Migration C++20, Sérialisation Binaire (`StreamWriter`/`StreamReader`) & `PacketDispatcher`

| Métadonnées | Détails |
| :--- | :--- |
| **Date** | `2026-11-14` |
| **Heure (Début → Fin)** | `11:00` → `18:47` |
| **Temps passé** | `7h 47m` |
| **Branche / Commit** | `network-logic` (`40a3f83`) |
| **Statut de fin de session** | 🟡 En cours (WIP) |

---

## 1. Contexte & Objectifs de la session
> *Où j'en étais en ouvrant le projet aujourd'hui, et qu'est-ce que je voulais accomplir pendant cette session ?*

En reprenant après la session #01 (`65dbcf3`), le transport `GodotUdpTransport` gérait déjà le multi-clients par `peer_id` virtuel, les timeouts et les événements `NetEvent`, mais `NetWorldCore::step()` envoyait encore directement tout paquet reçu vers le désérialiseur SoA (`deserialize_buffer`) sans distinction de type de message ni d'événement.

Avant de câbler le routage dans `NetWorldCore`, il fallait poser une couche de sérialisation binaire générique et typée (`StreamWriter` / `StreamReader`) ainsi qu'un routeur de messages (`PacketDispatcher`).

- [x] Rattraper et commiter le devlog #01 et la mise à jour du `README.md` (`13c4706`)
- [x] Passer le build SCons en C++20 pour exploiter concepts, `std::span` et `std::bit_cast` (`f45a801`, `00f407d`)
- [x] Définir les concepts de sérialisation réseau dans `NetStream.h` (`bf59ee4`, `407d1e7`)
- [x] Implémenter `StreamWriter` et `StreamReader` (`407d1e7`, `00f407d`)
- [x] Implémenter `PacketDispatcher` et `encode_packet` (`40a3f83`)
- [ ] Définir les structs de messages concrets (`knock`, `spawn`, `despawn`, `state_sync`) et brancher `PacketDispatcher` + `NetEvent` dans `NetWorldCore::step()`

---

## 2. Ce qui a été fait (Résumé des changements)
> *Liste claire des fonctionnalités ajoutées, modifiées ou supprimées.*

### Fichiers touchés
- `devlog/01_2026-11-13_transport_multi_clients.md` & `devlog/README.md` (`13c4706`) : rédaction et indexation du devlog de la veille.
- `SConstruct` (`f45a801`, `00f407d`) : activation du standard C++20 (`/std:c++20` sous Windows, `-std=c++20` sur les autres plateformes) avant la déclaration des cibles de compilation.
- `src/core/NetStream.h` (`bf59ee4`, `407d1e7`) : définition des concepts C++20 `NetIntegral`, `NetEnum`, `NetReadable`, `NetWritable` et `NetSerializable`.
- `src/core/StreamWriter.h` & `src/core/StreamWriter.cpp` (`407d1e7`) : sérialiseur binaire basé sur `std::vector<uint8_t>`.
- `src/core/StreamReader.h` & `src/core/StreamReader.cpp` (`00f407d`) : désérialiseur binaire sécurisé et zéro-copie basé sur `std::span<const uint8_t>` et `std::optional`.
- `src/core/PacketDispatcher.h` & `src/core/PacketDispatcher.cpp` (`40a3f83`) : concept `NetMessage`, helper `encode_packet<T>()`, et table de dispatch `PacketDispatcher`.

### Explication du fonctionnement
- **Contrats C++20 (`NetStream.h`) :**
  - `NetIntegral` contraint les entiers (`std::integral<T>`) tout en excluant explicitement `bool` (`!std::same_as<T, bool>`).
  - `NetEnum` cible les énumérations (`std::is_enum_v<T>`).
  - `NetWritable<T, Writer>` exige `obj.serialize(w) -> void` et `NetReadable<T, Reader>` exige `T::deserialize(r) -> std::optional<T>`. `NetSerializable<T, Reader, Writer>` combine les deux.
- **Écriture & lecture binaire (`StreamWriter` / `StreamReader`) :**
  - `StreamWriter` écrit les entiers via `std::memcpy`, les flottants via `std::bit_cast<uint32_t>`, les `Vec3` composant par composant, les enums via `std::underlying_type_t<T>`, les octets bruts/préfixés par taille (`uint32_t`), et toute structure satisfaisant `NetWritable<T, StreamWriter>` via `write_struct()`. `finish()` transfère le buffer par `std::move` et vide l'état interne.
  - `StreamReader` parcourt une vue `std::span<const uint8_t>` (`_cursor`) en avançant avec `subspan()`. Chaque lecture vérifie la taille restante et retourne `std::nullopt` en cas de paquet tronqué ou malformé.
- **Encodage et routage (`PacketDispatcher`) :**
  - Le concept `NetMessage<T>` impose `NetSerializable<T, StreamReader, StreamWriter>` ainsi qu'une constante `T::PACKET_ID` convertible en `uint8_t`.
  - `encode_packet(const T& msg)` préfixe automatiquement le payload avec `T::PACKET_ID` (1 octet) puis appelle `msg.serialize(writer)`.
  - `PacketDispatcher` stocke 256 handlers dans un `std::array<RawHandler, 256>` indexé par l'ID de paquet. `register_handler<T>(cb)` contraint le callback avec `std::invocable<Callback, uint32_t, const T&>`, désérialise `T::deserialize(reader)` dans le wrapper, puis appelle `cb(peer_id, *msg)` si la lecture réussit. `dispatch(peer_id, raw_packet)` lit le premier octet et exécute le handler correspondant en $O(1)$.

---

## 3. Choix Techniques & Architecture (Le "Pourquoi")
> *Pourquoi avoir codé ça de cette manière plutôt qu'une autre ? Quels compromis (performance, lisibilité, découplage) ont été faits ?*

- **Décision 1 : Migration vers C++20 (Concepts, `std::span`, `std::bit_cast`)**
  - *Alternative envisagée :* Rester en C++17 avec SFINAE (`std::enable_if`), des pointeurs bruts `(const uint8_t*, size_t)` et des conversions via `memcpy` / `union`.
  - *Raison du choix :* Les concepts rendent les contraintes sur les messages et types primitifs beaucoup plus lisibles, `std::span` évite les copies et sécurise la lecture par tranches (`subspan`), et `std::bit_cast` exprime proprement la réinterprétation `float` ↔ `uint32_t` sans UB.
- **Décision 2 : Séparation de `NetSerializable` en `NetReadable` et `NetWritable`**
  - *Alternative envisagée :* Garder un seul concept `NetSerializable<T, Reader, Writer>` partout (comme dans la première version de `NetStream.h`).
  - *Raison du choix :* `StreamWriter` n'a pas besoin de connaître `StreamReader` (et inversement). Séparer les deux évite une dépendance croisée entre `StreamWriter.h` et `StreamReader.h` tout en permettant à `write_struct` et `read_struct` de vérifier uniquement la capacité requise.
- **Décision 3 : Table de dispatch statique `std::array<RawHandler, 256>` indexée par `uint8_t`**
  - *Alternative envisagée :* Utiliser une `std::unordered_map<uint8_t, RawHandler>` ou un `switch` manuel dans `NetWorldCore`.
  - *Raison du choix :* Un identifiant sur 1 octet donne au maximum 256 types de paquets ; un tableau fixe garantit un lookup en $O(1)$ sans allocation ni hash, tout en découplant l'enregistrement des handlers de la boucle de réception.

### Schéma / Flux
```mermaid
flowchart LR
    M["Message (NetMessage T)"] -->|"encode_packet(msg)"| W["StreamWriter (PACKET_ID + serialize)"]
    W -->|"std::vector<uint8_t>"| NET["Transport UDP"]
    NET -->|"std::span<const uint8_t>"| D["PacketDispatcher::dispatch()"]
    D -->|"Lit PACKET_ID (1 octet)"| H["_handlers[id]"]
    H -->|"T::deserialize(StreamReader)"| CB["Callback(peer_id, const T&)"]
```

---

## 4. Problèmes rencontrés & Solutions (Debugging)
> *Qu'est-ce qui a posé problème ou fait perdre du temps aujourd'hui, et comment ça a été résolu ?*

- **Problème 1 : Prise en main des fonctionnalités C++20 et configuration SCons**
  - *Cause :* Découverte de l'utilisation pratique des fonctionnalités C++20 (`<concepts>`, `std::span`, `std::bit_cast`, `std::invocable`) et placement initial du bloc `CXXFLAGS` (`/std:c++20` / `-std=c++20`) à la fin de `SConstruct`, *après* les appels `env.SharedLibrary(...)`.
  - *Solution :* Déplacement de la configuration C++20 dans `SConstruct` avant la création des cibles (`00f407d`), ce qui a activé le compilateur en mode C++20 pour les sources du projet.
- **Problème 2 : Syntaxe des templates et des clauses `requires` C++20**
  - *Cause :* La syntaxe des expressions `requires` imbriquées, des contraintes de retour (`{ expr } -> std::same_as<...>`), de la séparation `NetReadable` / `NetWritable`, et de l'écriture de templates contraints (`template <NetMessage T, typename Callback> requires std::invocable<...>`) a demandé plusieurs itérations pour satisfaire le compilateur et garder des signatures propres entre `NetStream.h`, `StreamWriter.h`, `StreamReader.h` et `PacketDispatcher.h`.
  - *Solution :* Factorisation progressive des concepts dans `NetStream.h` (`NetReadable` / `NetWritable` / `NetSerializable`), utilisation de `requires` sur la signature exacte de `T::deserialize(r)` et `T::PACKET_ID`, et validation du pattern de capture `[func = std::forward<Callback>(cb)]` dans `register_handler`.

---

## 5. Où j'en suis & Prochaine Session (Reprise rapide)
> *LA section à relire en premier la prochaine fois que j'ouvre le projet !*

- **État exact du code en quittant :**
  - Les briques de base (`NetStream.h`, `StreamWriter`, `StreamReader`, `PacketDispatcher`) sont écrites et commitées sur `network-logic` (`40a3f83`).
  - Elles ne sont pas encore utilisées dans `NetWorldCore` : `NetworldCore.cpp` appelle toujours directement `deserialize_buffer()` sans vérifier `pkt.event` ni passer par `PacketDispatcher`.
  - Note : la dernière DLL debug visible dans `demo/bin/` date de `17:44` (avant le commit `40a3f83` de `18:34`) ; prévoir un rebuild complet avec SCons à la reprise pour valider la compilation finale avec `PacketDispatcher`.
- **Prochaines étapes immédiates (TODO de reprise) :**
  1. [ ] Lancer un build SCons pour vérifier que l'ensemble (y compris `PacketDispatcher`) compile proprement.
  2. [ ] Définir les structs de messages réseau satisfaisant `NetMessage` (ex. `Knock`, `SpawnEntity`, `DespawnEntity`, `StateSync` encapsulant les batches SoA) avec un enum `PacketId : uint8_t`.
  3. [ ] Dans `src/core/NetworldCore.cpp`, ajouter un `switch (pkt.event)` (`CONNECTED`, `DISCONNECTED`, `DATA`) dans `step()` et router `DATA` via `PacketDispatcher`.
  4. [ ] Implémenter la logique serveur/client pour `CONNECTED` / `DISCONNECTED` (spawn/despawn et synchronisation initiale des entités existantes).
- **Idées / Questions mises de côté pour plus tard :**
  - Ajouter éventuellement une vérification `reader.empty()` après `T::deserialize(reader)` dans `PacketDispatcher` (ou un mode debug) pour détecter d'éventuels octets résiduels.
  - Toujours en attente : gestion de la fiabilité pour `NetChannel::RELIABLE_EVENTS` (UDP brut sans ACK/resend pour l'instant).
