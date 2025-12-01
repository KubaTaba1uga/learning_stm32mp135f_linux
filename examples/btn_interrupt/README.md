We want to configure interrupts for User Button 1 (B1) and User Button 2 (B2).

B1 is connected to PA14 (GPIOA pin 14), so we need to edit the Device Tree to point to the correct pin and driver. We’ll use the `gpio-keys` driver, which translates GPIO button presses into input events (e.g., mapping B1 to ENTER).

In `input-event-codes.h`, `KEY_ENTER` is 28 and `KEY_ESC` is 1.

If the board boots correctly, we should see an input device under `/dev/input/`. When we `cat` that device and press B1 or B2, characters should appear.

Once we confirm the buttons work, we can read input events with our app. After logging into the board, run:
```bash
/root/btn_interrupt /dev/input/event0
```

You should see output like:
```bash
# /root/btn_interrupt /dev/input/event0
input_event {
  time:  946685030.063362
  type:  0x0001 (1)
  code:  0x001c (28)
  value: 0x00000001 (1)
}
# /root/btn_interrupt /dev/input/event0
input_event {
  time:  946686126.216997
  type:  0x0001 (1)
  code:  0x0001 (1)
  value: 0x00000001 (1)
}
```
