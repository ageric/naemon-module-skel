#/bin/sh

#mon qh query @nerd subscribe tcm | \
	gource - --log-format custom \
		--bloom-multiplier 0.5 \
		--bloom-intensity 0.4 \
		--user-scale 1.5 \
		--hide dirnames,filenames \
		--highlight-users
