This means that you need to read a Status Register (Alternate Status is a good choice) for the proper drive before sending the next command. Which means that you need to select the correct device first, before you can read that status

The suggestion is to read the Status register FIFTEEN TIMES, and only pay attention to the value returned by the last one -- after selecting a new master or slave device. The point being that you can assume an IO port read takes at least 30ns, so doing the first fourteen creates a 420ns delay -- which allows the drive time to push the correct voltages onto the bus.

have your driver remember the last value sent to each Drive Select IO port, to avoid doing unneeded drive selections, if the value did not change.

always needs to block if the current device is actively modifying BSY/DRQ/ERR, and your device driver always already knows that the device is in that condition (because the driver just sent the command to the device, and it hasn't been marked "complete" yet). Once a drive has actually completed a command, it will always clear BSY and DRQ. You can simply verify this, before your next Device Select command -- that the previously selected device cleared BSY and DRQ properly at command completion. Then you will never have to check if they are clear after a Device Select -- so you will not have to read the Status Register after the Device Select at all.

BSY and DRQ will be cleared, but ERR or DF remain set until just after you write a new command to the Command Register. If you are using polling (see below), you should account for the fact that your first four reads of the Status Register, after sending your command byte, may have the ERR or DF bits still set accidentally. (If you are using IRQs, the Status will always be correct by the time the IRQ is serviced.)

On some drives it is necessary to "manually" flush the hardware write cache after every write command. This is done by sending the 0xE7 command to the Command Register (then waiting for BSY to clear). If a driver does not do this, then subsequent write commands can fail invisibly, or "temporary bad sectors" can be created on your disk.
