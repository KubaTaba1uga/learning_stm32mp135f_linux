# Pinctrl Alternate Function

We are configuring USART1 on pin 8 and 10. 

To confirm it is working we have app wich tries to print and read back uart frames,
so to make it work connect pin 8 and pin 10 with jumper.

Once you are logged into board's console do:
```bash
# stty -F /dev/ttySTM1 115200 cs8 -cstopb -parenb -ixon -ixoff -crtscts raw
# sleep 5 && echo -n 'A' > /dev/ttySTM1 &
# hexdump -C < /dev/ttySTM1
```

Ans you should see some hex within few seconds.
