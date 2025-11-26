To show usb gadget on host you need to comment out `usb-role-swicth` in linux dts:
```
&usbotg_hs {
	phys = <&usbphyc_port1 0>;
	phy-names = "usb2-phy";
	/* usb-role-switch; */
	status = "okay";
	port {
		usbotg_hs_ep: endpoint {
			remote-endpoint = <&con_usb_c_g0_ep>;
		};
	};
};
```
