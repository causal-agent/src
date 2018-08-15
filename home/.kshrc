set -o nounset -o noclobber -o braceexpand -o vi
HISTFILE=~/.ksh_history HISTSIZE=5000

function colon {
	IFS=:
	print "$*"
}
system_path=$PATH
PATH=$(colon {,/usr{/local,/pkg,},$HOME/.local}/{s,}bin /usr/games)

export PAGER=less MANPAGER=less EDITOR=vim GIT_EDITOR=vim
if type nvim > /dev/null; then
	EDITOR=nvim GIT_EDITOR=nvim MANPAGER="nvim -c 'set ft=man' -"
	alias vim=nvim
fi
export GPG_TTY=$(tty)

export CLICOLOR=1
# TODO: GNU aliases.

export NETHACKOPTIONS='
	name:June, role:Valkyrie, race:Human, gender:female, align:neutral,
	dogname:Moro, catname: Baron, pickup_types:$!?+/=,
	color, DECgraphics
'

alias ll='ls -lh'
alias gs='git status --short --branch' gd='git diff'
alias gsh='git show' gl='git log --graph --pretty=log'
alias gco='git checkout' gb='git branch' gm='git merge' gst='git stash'
alias ga='git add' gmv='git mv' grm='git rm'
alias gc='git commit' gca='gc --amend' gt='git tag'
alias gp='git push' gu='git pull' gf='git fetch'
alias gr='git rebase' gra='gr --abort' grc='gr --continue' grs='gr --skip'
alias rand='openssl rand -base64 33'

function colors {
	typeset sgr0=sgr0 setaf=setaf
	[[ -f /usr/share/misc/termcap ]] && sgr0=me setaf=AF
	set -A fg \
		$(tput $sgr0) \
		$(tput $setaf 1) \
		$(tput $setaf 2) \
		$(tput $setaf 3) \
		$(tput $setaf 4) \
		$(tput $setaf 5) \
		$(tput $setaf 6) \
		$(tput $setaf 7)
}
colors

function branch {
	typeset git=.git head
	[[ -d $git ]] || git=../.git
	[[ -d $git ]] || return
	read head < $git/HEAD
	if [[ $head = ref:* ]]; then
		print ":${head#*/*/}"
	else
		typeset -L 7 head
		print ":$head"
	fi
}

function prompt {
	typeset status=$? title left right path color cols

	[[ ${PWD#$HOME} != $PWD ]] && path="~${PWD#$HOME}" || path=$PWD
	title=${path##*/}
	right="$path$(branch)"

	[[ -n ${SSH_CLIENT:-} ]] && color=${fg[5]} || color=${fg[7]}
	(( status )) && color=${fg[1]}
	left="\01$color\01\$\01$fg\01 "

	[[ $TERM = xterm* ]] && title="\033]0;$title\07" || title=''
	[[ -n ${COLUMNS:-} ]] && cols=$COLUMNS || cols=$(tput cols)
	typeset -R $(( cols / 2 )) right
	typeset -R $(( cols - 1 )) right
	print "\01\r\01$title\01\n\01${fg[7]}\01$right\01$fg\01\r$left"
}
PS1='$(prompt)'
