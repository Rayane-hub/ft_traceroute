# Nom du fichier exécutable qui sera créé sur le disque
NAME	= ft_traceroute

# S'assurer que 'all' est l'objectif par défaut
.DEFAULT_GOAL := all

# Fichier(s) .c présents dans le projet
SRCS	= src/main.c

# Programme utilisé pour compiler (gcc)
CC		= gcc

INC_DIR	= src

# Options passées à gcc lors de chaque compilation
CFLAGS 	= -Wall -Wextra -Werror -I$(INC_DIR) -MMD -MP

# Définir le Makefile comme fichier (utile pour forcer recompilation si on le modifie)
MAKEFILE := Makefile

# Dossier de build caché placé dans le répertoire `src`
OBJDIR := src/.build

# Liste des fichiers .o et .d générés dans $(OBJDIR)
# Convertit src/xxx.c -> src/.build/xxx.o
OBJS	= $(patsubst src/%.c,$(OBJDIR)/%.o,$(SRCS))
DEPS	= $(OBJS:.o=.d)

# Si le Makefile change, forcer la régénération des .o
$(OBJS): $(MAKEFILE)

# Make vérifie si le fichier ft_traceroute existe et est à jour sinon le creer
all: $(NAME)

# Inclure les fichiers .d (ils sont dans $(OBJDIR))
-include $(DEPS)

# Commande exécutée pour créer le fichier ft_traceroute
# Elle assemble tous les fichiers .o en un exécutable
$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -lm -o $(NAME)

# Commande exécutée pour créer un fichier .o à partir d’un fichier .c
# gcc compile le .c sans créer d’exécutable (-c)
# Règle : compiler un src/%.c en src/.build/%.o
# Ex: src/.build/main.o : src/main.c
$(OBJDIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Supprime uniquement les fichiers .o du disque
clean:
	rm -rf $(OBJDIR)

# Supprime les fichiers .o et l’exécutable ft_traceroute
# fclean: s'assure d'appeler clean puis supprime l'exécutable et les .d
fclean: clean
	rm -f $(NAME)
	
# Supprime tout puis relance la compilation complète
re: fclean all

.PHONY: all clean fclean re
