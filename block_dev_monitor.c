#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <string.h>

#include <libudev.h>

int main(void)
{
    struct udev *udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    struct udev_device *dev;

    struct udev_monitor *mon;
    struct timeval tv;
    fd_set fds;
    int fd;
    int ret;

    const char *path;

    const char *tmp;

    udev = udev_new();
    if (!udev) {
        printf("Can't create udev\n");
        exit(1);
    }

    mon = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "block", NULL);
    fd = udev_monitor_get_fd(mon);

    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_scan_devices(enumerate);
    devices =udev_enumerate_get_list_entry(enumerate);

    udev_list_entry_foreach(dev_list_entry, devices) {
        path = udev_list_entry_get_name(dev_list_entry);
        dev = udev_device_new_from_syspath(udev, path);

        if (strncmp(udev_device_get_devtype(dev), "partition", 9) == 0 ||
            strncmp(udev_device_get_sysname(dev), "sd", 2) != 0)
            continue;

        printf("==============================\n");
        printf("Device Node Path : %s\n", udev_device_get_devnode(dev));
        printf("==============================\n");
        printf("SERIAL  : %s\n", udev_device_get_property_value(dev, "ID_SERIAL_SHORT"));
        printf("MODEL   : %s\n", udev_device_get_property_value(dev, "ID_MODEL"));
        printf("VENDOR  : %s\n", udev_device_get_property_value(dev, "ID_VENDOR"));
        printf("DEVTYPE : %s\n", udev_device_get_devtype(dev));

        tmp = udev_device_get_property_value(dev, "ID_BUS");
        if (tmp != NULL) {
            if (strncmp(tmp, "ata", 3) == 0) {
                tmp = udev_device_get_property_value(dev, "ID_ATA_SATA");
                if ((tmp != NULL) && strncmp(tmp, "1", 1) == 0)
                    printf("BUSTYPE : sata\n");
            }
            else if (strncmp(tmp, "usb", 3) == 0)
                printf("BUSTYPE : usb\n");

        }
        printf("\n");

        udev_device_unref(dev);
    }
    udev_enumerate_unref(enumerate);

    if (udev_monitor_enable_receiving(mon) != 0)
        printf("enabling udev monitor has been filed\n");

    while (1) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        ret = select(fd+1, &fds, NULL, NULL, &tv);

        if (ret > 0 && FD_ISSET(fd, &fds)) {
            dev = udev_monitor_receive_device(mon);

            if (dev) {
                if (strncmp(udev_device_get_devtype(dev), "partition", 9) == 0)
                    continue;

                printf("\n%s ", udev_device_get_devnode(dev));
                tmp = udev_device_get_action(dev);
                if (tmp != NULL) {
                    if (strncmp(tmp, "add", 3) == 0)
                        printf("has been added\n");
                    else if (strncmp(tmp, "remove", 6) == 0)
                        printf("has been removed\n");
                    else if (strncmp(tmp, "change", 6) == 0)
                        printf("has been changed\n");
                    else if (strncmp(tmp, "online", 6) == 0)
                        printf("is online\n");
                    else if (strncmp(tmp, "offline", 6) == 0)
                        printf("is offline\n");
                }
            }
            else {
                printf("No Device from receive_device(). An error occured.\n");
            }
        }
        usleep(250*1000);
        printf(".");
        fflush(stdout);
    }

    udev_monitor_unref(mon);
    udev_unref(udev);

    return 0;
}
