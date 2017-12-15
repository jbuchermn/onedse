WEBPACK = $(PWD)/node_modules/.bin/webpack
VULCANIZE = $(PWD)/node_modules/.bin/vulcanize

.PHONY: default
default: www/index.html

www/bundle.js: $(shell find src -type f)
	mkdir -p www
	$(WEBPACK) -d

www/index.html: index.html www/bundle.js
	$(VULCANIZE) --inline-scripts index.html > www/index.html

.PHONY: clean
clean:
	rm -rf www
