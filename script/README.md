# IRQ pinning

The PCO acquisition thread in Lima runs with `SCHED_FIFO` and might compete with the SDK thread that handles the frame-grabber IRQ.

To avoid that situation, one needs to
- pin the frame grabber's interrupt to a CPU.
- make sure that the IRQ balancing does not attribute the same cpu to and another device.

First, one needs to select a CPU that is on the proper NUMA node - the same one as the frame grabber's PCI device (`acq_thread_numa_node` in the Tango device properties). Given the PCI address:

```
cat /sys/bus/pci/devices/0000:86:00.0/numa_node
cat /sys/devices/system/node/node$(cat /sys/bus/pci/devices/0000:86:00.0/numa_node)/cpulist
```

In our example, we have `numa_node = 1` and the associated CPUs `1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31`. We will pick `3` (the second CPU of that node) as `TARGET_CPU`, avoiding the first one (CPU 1) since that's more likely to already have some other system process pinned to it.

## IRQ pin doesn't survive reboot/driver reload — needs a boot-time script.

```
sudo cp pin-predator-irq.sh /usr/local/sbin/pin-predator-irq.sh
sudo chmod 755 pin-predator-irq.sh

sudo cp predator-irq-pin.default /etc/default/predator-irq-pin
sudo cp predator-irq-pin.service /etc/systemd/system/predator-irq-pin.service

# Enable and start the daemon
sudo systemctl daemon-reload
sudo systemctl enable --now predator-irq-pin.service

```

## `irqbalance` could still reclaim CPU n later unless it's banned from managing it.

Edit `/etc/default/irqbalance`

```
IRQBALANCE_BANNED_CPULIST=3
sudo systemctl restart irqbalance
```

To check after reboot:

```
journalctl -t predator-irq-pin
cat /proc/irq/$(cat /sys/bus/pci/devices/0000:86:00.0/irq)/smp_affinity_list
```

should show `3`.