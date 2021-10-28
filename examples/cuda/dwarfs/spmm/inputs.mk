ifndef APPLICATION_PATH
$(error "APPLICATION_PATH not defined. Please define this variable")
endif
INPUTS_DIR = $(APPLICATION_PATH)/inputs

###################
# Wiki-Vote graph #
###################
INPUTS                 += wiki-vote
wiki-vote		= $(INPUTS_DIR)/wiki-Vote/wiki-Vote.mtx
wiki-vote__directed	= yes
wiki-vote__weighted	= no
wiki-vote__zero-indexed = no
wiki-vote__rows         = 8297
wiki-vote__cols         = 8297
wiki-vote__nnz          = 103689
$(wiki-vote):  url=https://suitesparse-collection-website.herokuapp.com/MM/SNAP/wiki-Vote.tar.gz
$(wiki-vote):  tar=wiki-Vote.tar.gz

##############
# poisson-3D #
##############
INPUTS              	       += poisson-3D
poisson-3D			= $(INPUTS_DIR)/poisson3Da/poisson3Da.mtx
poisson-3D__directed		= yes
poisson-3D__weighted		= yes
poisson-3D__zero-indexed	= no
poisson-3D__rows 		= 13514
poisson-3D__cols 		= 13514
poisson-3D__nnz 		= 352762
$(poisson-3D): url=https://suitesparse-collection-website.herokuapp.com/MM/FEMLAB/poisson3Da.tar.gz
$(poisson-3D): tar=poisson3Da.tar.gz

##############
# ca-CondMat #
##############
INPUTS              	       += ca-CondMat
ca-CondMat			= $(INPUTS_DIR)/ca-CondMat/ca-CondMat.mtx
ca-CondMat__directed		= no
ca-CondMat__weighted		= no
ca-CondMat__zero-indexed	= no
ca-CondMat__rows 		= 23133
ca-CondMat__cols 		= 23133
ca-CondMat__nnz 		= 186936
$(ca-CondMat): url=https://suitesparse-collection-website.herokuapp.com/MM/SNAP/ca-CondMat.tar.gz
$(ca-CondMat): tar=ca-CondMat.tar.gz

##############
# offshore #
##############
INPUTS              	       += offshore
offshore			= $(INPUTS_DIR)/offshore/offshore.mtx
offshore__directed		= no
offshore__weighted		= yes
offshore__zero-indexed	= no
offshore__rows 		= 259789
offshore__cols 		= 259789
offshore__nnz 		= 4242673
$(offshore): url=https://suitesparse-collection-website.herokuapp.com/MM/Um/offshore.tar.gz
$(offshore): tar=offshore.tar.gz

############
# 08blocks #
############
INPUTS += 08blocks
08blocks = $(INPUTS_DIR)/08blocks/08blocks.mtx
08blocks__directed = yes
08blocks__weighted = yes
08blocks__zero-indexed = no
08blocks__rows = 300
08blocks__cols = 300
08blocks__nnz  = 592
$(08blocks): url=https://suitesparse-collection-website.herokuapp.com/MM/JGD_SPG/08blocks.tar.gz
$(08blocks): tar=08blocks.tar.gz

##############
# roadNet-CA #
##############
INPUTS += roadNet-CA
roadNet-CA = $(INPUTS_DIR)/roadNet-CA/roadNet-CA.mtx
roadNet-CA__directed = no
roadNet-CA__weighted = no
roadNet-CA__zero-indexed = no
roadNet-CA__rows = 1971281
roadNet-CA__cols = 1971281
roadNet-CA__nnz  = 5533214
$(roadNet-CA): url=https://suitesparse-collection-website.herokuapp.com/MM/SNAP/roadNet-CA.tar.gz
$(roadNet-CA): tar=roadNet-CA.tar.gz

################
# road_central #
################
INPUTS += road-central
road-central = $(INPUTS_DIR)/road_central/road_central.mtx
road-central__directed = no
road-central__weighted = no
road-central__zero-indexed = no
road-central__rows = 14081816
road-central__cols = 14081816
road-central__nnz  = 33866826
$(road-central): url=https://suitesparse-collection-website.herokuapp.com/MM/DIMACS10/road_central.tar.gz
$(road-central): tar=road_central.tar.gz

############
# road_usa #
############
INPUTS += road_usa
road-usa = $(INPUTS_DIR)/road_usa/road_usa.mtx
road-usa__directed = no
road-usa__weighted = no
road-usa__zero-indexed = no
road-usa__rows = 23947347
road-usa__cols = 23947347
road-usa__nnz  = 57708624
$(road-usa): url=https://suitesparse-collection-website.herokuapp.com/MM/DIMACS10/road_usa.tar.gz
$(road-usa): tar=road_usa.tar.gz

#########
# Pokec #
#########
INPUTS += soc-Pokec
soc-Pokec = $(INPUTS_DIR)/soc-Pokec/soc-Pokec.mtx
soc-Pokec__directed = yes
soc-Pokec__weighted = no
soc-Pokec__zero-indexed = no
soc-Pokec__rows =  1632803
soc-Pokec__cols =  1632803
soc-Pokec__nnz  = 30622564
$(soc-Pokec): url=https://suitesparse-collection-website.herokuapp.com/MM/SNAP/soc-Pokec.tar.gz
$(soc-Pokec): tar=soc-Pokec.tar.gz

##############
# Hollywood  #
##############
INPUTS += hollywood
hollywood = $(INPUTS_DIR)/hollywood-2009/hollywood-2009.mtx
hollywood__directed = yes
hollywood__weighted = no
hollywood__zero-indexed = no
hollywood__rows =   1139905
hollywood__cols =   1139905
hollywood__nnz  = 113891327
$(hollywood): url=https://suitesparse-collection-website.herokuapp.com/MM/LAW/hollywood-2009.tar.gz
$(hollywood): tar=hollywood-2009.tar.gz

###############
# LiveJournal #
###############
INPUTS += LiveJournal
LiveJournal = $(INPUTS_DIR)/ljournal-2008/ljournal-2008.mtx
LiveJournal__directed = yes
LiveJournal__weighted = no
LiveJournal__zero-indexed = no
LiveJournal__rows =  5363260
LiveJournal__cols =  5363260
LiveJournal__nnz  = 79023142
$(LiveJournal): url=https://suitesparse-collection-website.herokuapp.com/MM/LAW/ljournal-2008.tar.gz
$(LiveJournal): tar=ljournal-2008.tar.gz

# Download and unpack
$(foreach i,$(INPUTS),$($i)):
	@echo "Downloading and unpacking $@"
	@mkdir -p $(INPUTS_DIR)
	@cd $(INPUTS_DIR) && wget $(url)
	@cd $(INPUTS_DIR) && tar zxf $(tar)
	@touch $@ # updates the timecode of the untared file

inputs: $(foreach i,$(INPUTS),$($i))

clean.inputs:
	rm -rf $(INPUTS_DIR)
