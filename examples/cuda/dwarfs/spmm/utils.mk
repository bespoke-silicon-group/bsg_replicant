# statically computed values
log2 = $(shell echo 'l($(1))/l(2)' | bc -l | xargs printf "%.f\n")

# range
range = $(shell echo {$1..$(shell echo $2-1|bc)})

# cartesion product of two lists
cartesian = $(foreach x,$1,$(foreach y,$2,$x$3$y))

# decouple list with delimiteer
unpack = $(subst $2, ,$1)

test_range:
	$(eval r_0_10 = $(call range,0,10))
	@echo "r_0_10 = $(r_0_10)"

test_cartesian:
	$(eval cart = $(call cartesian,apple banana,green yellow,_))
	@echo "cart = $(cart)"
	$(eval cart = $(call cartesian,$(call range,0,10),$(call range,0,10),x))
	@echo "cart = $(cart)"
	@echo "unpacked(cart[0]) = $(call unpack,$(firstword $(cart)),x)"
