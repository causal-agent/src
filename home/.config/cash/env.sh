set -o noclobber -o nounset -o vi

colon() {
	IFS=:
	echo "$*"
}

# {,/opt/pkg,/usr{/local,/pkg,},$HOME/.local}/{s,}bin /usr/games
_PATH=$PATH
PATH=$(colon /sbin /bin /opt/pkg/sbin /opt/pkg/bin /usr/local/sbin /usr/local/bin /usr/pkg/sbin /usr/pkg/bin /usr/sbin /usr/bin ~/.local/sbin ~/.local/bin /usr/games)
CDPATH=:~

export PAGER=less
export EDITOR=nvim
export MANPAGER="nvim -c 'set ft=man' -"
export MANSECT=2:3:1:8:6:5:7:4:9
export CLICOLOR=1
export GPG_TTY=$(tty)
export NETHACKOPTIONS='pickup_types:$!?+/=, color, DECgraphics'
type nvim > /dev/null || EDITOR=vim

alias vim="$EDITOR"
alias ls='ls -p'
alias ll='ls -hl'
alias bc='bc -l'
alias gs='git status --short --branch || ls' gd='git diff'
alias gsh='git show' gl='git log --graph --pretty=log'
alias gco='git checkout' gb='git branch' gm='git merge' gst='git stash'
alias ga='git add' gmv='git mv' grm='git rm'
alias gc='git commit' gca='gc --amend' gt='git tag'
alias gp='git push' gu='git pull' gf='git fetch'
alias gr='git rebase' gra='gr --abort' grc='gr --continue' grs='gr --skip'
alias rand='openssl rand -base64 33'
alias private='eval "$(gpg -d ~/.private)"'
[ "$(uname)" = 'Linux' ] && alias ls='ls --color=auto' grep='grep --color'

af0=$(tput setaf 0 || tput AF 0)
af7=$(tput setaf 7 || tput AF 7)
sgr0=$(tput sgr0 || tput me)

PSlit=$'\1'
[ "${TERM%-*}" = 'xterm' ] && PS0=$'\e]0;\\W\a\n' || PS0=$'\n'
PS1="${PSlit}${af7}${PSlit}\\\$${PSlit}${sgr0}${PSlit} "
RPS1="${PSlit}${af0}${PSlit}# ${PSlit}${af7}${PSlit}\\w${PSlit}${sgr0}${PSlit}"
