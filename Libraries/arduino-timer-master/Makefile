NAME := arduino-timer
VERSION := $(shell git describe --tags --always --dirty)

$(NAME)-$(VERSION).zip:
	git archive HEAD --prefix=$(@:.zip=)/ --format=zip -o $@

tag:
	git tag $(VERSION)
