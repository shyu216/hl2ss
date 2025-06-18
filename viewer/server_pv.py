
import cv2
import hl2ss
import hl2ss_lnm

host = None

hl2ss_lnm.start_subsystem_pv(host, hl2ss.StreamPort.PERSONAL_VIDEO)

with hl2ss_lnm.rx_pv(host, hl2ss.StreamPort.PERSONAL_VIDEO) as client:
    while (True):
        data = client.get_next_packet()

        cv2.imshow('Video', data.payload.image)
        if ((cv2.waitKey(1) & 0xFF) == 27):
            break

hl2ss_lnm.stop_subsystem_pv(host, hl2ss.StreamPort.PERSONAL_VIDEO)
