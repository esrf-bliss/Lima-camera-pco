#!/bin/bash
# Continuously keeps the frame grabber's MSI interrupt pinned to a fixed CPU,
# so it can never collide with the Lima acquisition thread's SCHED_FIFO core
# and can't be moved by irqbalance.
#
# This can't be a one-shot action triggered at driver bind: predator_driver
# does not request its real IRQ vector at all until an application actually
# opens the frame grabber, which can happen anytime after boot (not at a
# fixed, udev-observable moment) - a bounded retry-at-bind-time script can
# miss it entirely if nothing touches the camera within the retry window.
# So this polls indefinitely instead, and also self-heals if the affinity
# ever gets reset for any other reason (e.g. a driver reload assigning a new
# IRQ number).
set -u

# Overridable via /etc/default/predator-irq-pin
PCI_ADDR="${PCI_ADDR:-0000:86:00.0}"
TARGET_CPU="${TARGET_CPU:-3}"
POLL_INTERVAL="${POLL_INTERVAL:-5}"

if [ -f /etc/default/predator-irq-pin ]; then
    # shellcheck disable=SC1091
    . /etc/default/predator-irq-pin
fi

IRQ_FILE="/sys/bus/pci/devices/${PCI_ADDR}/irq"

while true; do
    if [ -r "$IRQ_FILE" ]; then
        irq_num=$(cat "$IRQ_FILE" 2>/dev/null || true)
        if [ -n "${irq_num:-}" ] && [ "$irq_num" != "0" ]; then
            affinity_file="/proc/irq/${irq_num}/smp_affinity_list"
            if [ -w "$affinity_file" ]; then
                current=$(cat "$affinity_file" 2>/dev/null || true)
                if [ "$current" != "$TARGET_CPU" ]; then
                    echo "${TARGET_CPU}" > "$affinity_file"
                    logger -t predator-irq-pin "pinned IRQ ${irq_num} (PCI ${PCI_ADDR}) to CPU ${TARGET_CPU} (was '${current}')"
                fi
            fi
        fi
    fi
    sleep "$POLL_INTERVAL"
done
