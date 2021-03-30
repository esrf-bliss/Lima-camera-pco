# Makefile for lib and demo programs

all:	libpcocam pco_camera_grab pco_ser_con


clean:	FORCE
	cd ./pco_classes && $(MAKE) clean
	cd ./pco_ser_con && $(MAKE) clean
	cd ./pco_camera_grab && $(MAKE) clean
	$(RM) *.log *~ ./bin/*.log


libpcocam: FORCE
	cd ./pco_classes && $(MAKE)
	./symlink_pco -b

pco_ser_con: FORCE
	cd ./pco_ser_con && $(MAKE)

pco_camera_grab: FORCE
	cd ./pco_camera_grab && $(MAKE)


FORCE:
