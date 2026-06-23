# ft_traceroute

## Description

**ft_traceroute** est une réimplémentation du programme `traceroute` sous Linux.

Le projet consiste à découvrir le chemin emprunté par les paquets IP entre une machine source et une destination distante en exploitant le champ TTL (Time To Live) du protocole IP et les messages ICMP générés par les routeurs intermédiaires.

L'objectif est de comprendre le fonctionnement du routage IP, du TTL, des messages ICMP ainsi que la programmation réseau bas niveau sous Linux.

---

## Objectifs

- Comprendre le fonctionnement du routage IP.
- Manipuler les sockets réseau.
- Utiliser le champ TTL des paquets IP.
- Interpréter les réponses ICMP.
- Identifier les routeurs intermédiaires.
- Mesurer les temps de réponse du réseau.
- Approfondir les concepts du protocole TCP/IP.

---

## Fonctionnalités

- Résolution DNS.
- Découverte du chemin réseau vers une destination.
- Incrémentation progressive du TTL.
- Réception et analyse des réponses ICMP.
- Affichage des routeurs traversés.
- Mesure du RTT pour chaque saut.
- Gestion des erreurs réseau.
- Gestion des timeouts.
- Affichage similaire à la commande traceroute.

---

## Structure du projet

```text
ft_traceroute/
├── include/
│   ├── ft_traceroute.h
│   ├── network.h
│   └── utils.h
│
├── src/
│   ├── main.c
│   ├── traceroute.c
│   ├── packet.c
│   ├── receive.c
│   ├── dns.c
│   ├── socket.c
│   ├── statistics.c
│   └── utils.c
│
├── Makefile
└── README.md
```

---

## Compilation

Compiler le projet :

```bash
make
```

Nettoyer les fichiers objets :

```bash
make clean
```

Nettoyer complètement :

```bash
make fclean
```

Recompiler :

```bash
make re
```

---

## Utilisation

```bash
sudo ./ft_traceroute <host>
```

Exemples :

```bash
sudo ./ft_traceroute google.com
```

```bash
sudo ./ft_traceroute 8.8.8.8
```

```bash
sudo ./ft_traceroute localhost
```

---

## Exemple de sortie

```text
$ sudo ./ft_traceroute google.com

traceroute to google.com (142.250.179.238), 30 hops max

 1  192.168.1.1       1.124 ms   1.201 ms   1.089 ms
 2  10.0.0.1         10.842 ms  10.512 ms  10.791 ms
 3  172.16.0.1       15.024 ms  15.218 ms  14.997 ms
 4  142.250.179.238  16.420 ms  16.331 ms  16.287 ms
```

---

## Fonctionnement

Le programme envoie plusieurs paquets avec un TTL croissant :

```text
TTL = 1
TTL = 2
TTL = 3
...
```

Lorsqu'un routeur reçoit un paquet dont le TTL atteint zéro, il renvoie un message :

```text
ICMP Time Exceeded
```

Le programme récupère alors l'adresse IP du routeur ayant généré cette réponse.

Lorsque la destination finale est atteinte, elle répond généralement par :

```text
ICMP Port Unreachable
```

ou

```text
ICMP Echo Reply
```

selon la méthode utilisée.

---

## Time To Live (TTL)

Le champ TTL limite la durée de vie d'un paquet IP.

À chaque passage sur un routeur :

```text
TTL = TTL - 1
```

Lorsque :

```text
TTL = 0
```

le paquet est détruit et un message ICMP est envoyé à l'émetteur.

---

## ICMP

### Time Exceeded

```text
Type : 11
Code : 0
```

Indique qu'un routeur a supprimé un paquet dont le TTL a expiré.

### Echo Reply

```text
Type : 0
Code : 0
```

Réponse classique d'une destination atteinte.

### Destination Unreachable

```text
Type : 3
```

Utilisé par certaines implémentations de traceroute.

---

## RTT (Round Trip Time)

Le RTT correspond au temps nécessaire pour :

1. Envoyer un paquet.
2. Recevoir une réponse ICMP.

Calcul :

```text
RTT = temps_réception - temps_envoi
```

Exemple :

```text
15.423 ms
```

---

## Gestion des erreurs

### Hôte introuvable

```text
ft_traceroute: unknown host
```

### Permissions insuffisantes

```text
socket: Operation not permitted
```

### Timeout

```text
*
```

ou

```text
Request timed out
```

---

## Concepts abordés

### Réseau

- IPv4
- ICMP
- TTL
- Routage IP
- DNS

### Système

- Sockets
- RAW Sockets
- recvfrom()
- sendto()
- setsockopt()

### Programmation

- Structures réseau
- Gestion mémoire
- Analyse de paquets
- Gestion d'erreurs

---

## Comparaison avec traceroute

| Fonctionnalité | traceroute | ft_traceroute |
|---------------|------------|---------------|
| Découverte des sauts | ✅ | ✅ |
| Gestion TTL | ✅ | ✅ |
| Analyse ICMP | ✅ | ✅ |
| RTT | ✅ | ✅ |
| DNS | ✅ | ✅ |
| IPv6 | ✅ | Selon sujet |
| Options avancées | ✅ | Partiel |

---

## Ressources

- RFC 791 — Internet Protocol
- RFC 792 — Internet Control Message Protocol
- Linux Manual Pages

```bash
man traceroute
man socket
man recvfrom
man sendto
man ip
```

---

## Auteur

Projet réalisé dans le cadre du cursus de l'École 42.

**Rayane Samad**
