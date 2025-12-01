We want to configure interrupt on User button 1 (B1). 
B1 is connected to PA14 (GPIO A pin 15), so we need to edit device tree
to point to proper pin with proper driver. Driver of our choice is `gpio-keys`
which transferr button trigger to appropriate button (for example btn1 into ENTER).

In input-event-codes.h we can find that KEY_ENTER is 28.

If boot went well we should see a device in /dev/input/. If we cat this device and 
push B1 or B2 we should have a characters printed.


 


