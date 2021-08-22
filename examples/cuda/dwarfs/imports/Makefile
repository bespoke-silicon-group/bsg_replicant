repos += graph-tools
repos += hammerblade-helpers
repos += eigen

graph-tools.url		= git@github.com:mrutt92/graph-tools
hammerblade-helpers.url = git@github.com:mrutt92/hammerblade-helpers
eigen.url               = https://gitlab.com/libeigen/eigen.git

graph-tools.commit		= master
hammerblade-helpers.commit	= master
eigen.commit                    = 3.3.9

all: $(repos)

$(repos):
	$(eval url=$($@.url))
	$(eval commit=$($@.commit))
	git clone $(url) --recursive
	cd $@ && git checkout $(commit)

clean:
	rm -rf $(repos)
