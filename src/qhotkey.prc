mac: LIBS += -framework Carbon
else:win32: LIBS += -luser32
else:unix {
	LIBS += -lX11 -lxcb
}
