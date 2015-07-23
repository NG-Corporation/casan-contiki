# casan-contiki


	Avant de pouvoir lancer les tests, il faut d'abord adapter la variable CONTIKI
dans les Makefile, la variable CONTIKI doit être pointer vers la racine du Contiki
iot-lab/parts/contiki.

	Il y a 3 fichiers dans le dossier fich_Compl à déplacer :
		-	Makefile.include, radio-rf2xx.c dans iot-lab/parts/contiki/platform/openlab
		- 	rf2xx.h dans  iot-lab/parts/contiki/platform/openlab/periph (dossier à créer si existe pas)

