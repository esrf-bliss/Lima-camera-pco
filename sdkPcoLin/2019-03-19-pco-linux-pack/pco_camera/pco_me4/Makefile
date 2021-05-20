# Makefile for demo programs


all:	libpcocam pco_camera_grab pco_edge_grab pco_edge_grab_gs pco_ser_con


clean:	FORCE
	cd ./pco_classes && $(MAKE) clean
	cd ./pco_ser_con && $(MAKE) clean
	cd ./pco_camera_grab && $(MAKE) clean
	cd ./pco_edge_grab && $(MAKE) clean
	cd ./pco_edge_grab_exttrig && $(MAKE) clean
	cd ./pco_edge_grab_gl && $(MAKE) clean
	$(RM) *.log *~ ./bin/*.log

libpcocam: FORCE
	cd ./pco_classes && $(MAKE)
	./symlink_pco -b

pco_ser_con: FORCE
	cd ./pco_ser_con && $(MAKE)

pco_camera_grab: FORCE
	cd ./pco_camera_grab && $(MAKE)

pco_edge_grab: FORCE
	cd ./pco_edge_grab && $(MAKE)

pco_edge_grab_exttrig: FORCE
	cd ./pco_edge_grab_exttrig && $(MAKE)

pco_edge_grab_gs: FORCE
	cd ./pco_edge_grab_gl && $(MAKE)

FORCE:
