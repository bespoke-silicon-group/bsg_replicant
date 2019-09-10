TARGETS = library spmd cuda python

.PHONY: clean %.clean regression $(TARGETS)

clean: $(addsuffix .clean,$(TARGETS))
%.clean:
	$(MAKE) -C $(basename $@) clean

regression: $(TARGETS)
$(TARGETS): 
	$(MAKE) -C $@ regression

