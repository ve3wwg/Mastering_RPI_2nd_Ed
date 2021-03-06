
PROJECTS = dht11 ds3231 evinput gpio nunchuk spiloop # libusb

TSTAMP = $$(date '+%Y-%m-%d')

all:
	for dir in $(PROJECTS) ; do make -C $$dir all ; done

clean:
	for dir in $(PROJECTS) ; do make -C $$dir clean ; done

clobber:
	for dir in $(PROJECTS) libusb ; do make -C $$dir clobber ; done
	find . -name '*.t' -exec rm -f {} \;

tar:	clobber
	@rm -fr /tmp/stage /tmp/rasp-$(TSTAMP) rasp-$(TSTAMP).tar.gz
	@mkdir /tmp/stage
	cp -pdr * /tmp/stage/.
	@mv /tmp/stage /tmp/rasp-$$(date '+%Y-%m-%d')
	(cd /tmp && tar czf rasp-$$(date '+%Y-%m-%d').tar.gz rasp-$$(date '+%Y-%m-%d'))
	cp /tmp/rasp-$$(date '+%Y-%m-%d').tar.gz .
	@rm -fr /tmp/rasp-$$(date '+%Y-%m-%d')
	@rm -f /tmp/rasp-$$(date '+%Y-%m-%d').tar.gz
	ls -l rasp-$(TSTAMP).tar.gz
