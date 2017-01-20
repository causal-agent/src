unsetopt beep
setopt nomatch interactive_comments
setopt inc_append_history hist_ignore_dups
HISTFILE=~/.history HISTSIZE=5000 SAVEHIST=5000

autoload -Uz compinit && compinit
autoload -Uz colors && colors

bindkey -v
KEYTIMEOUT=1

OLDPATH=$PATH
path=(
  /sbin /bin
  /usr/local/sbin /usr/local/bin
  /usr/sbin /usr/bin
  ~/.bin ~/.cargo/bin ~/.gem/bin
)

export PAGER=less MANPAGER=less EDITOR=vim GIT_EDITOR=vim
type nvim > /dev/null \
  && MANPAGER=manpager EDITOR=nvim GIT_EDITOR=nvim \
  && alias vim=nvim
export GPG_TTY=$TTY

export CLICOLOR=1
[ "$OSTYPE" = 'linux-gnu' ] \
  && alias ls='ls --color=auto' grep='grep --color' rm='rm -I'

alias gs='git status --short --branch' gd='git diff'
alias gsh='git show' gl='git log --graph --pretty=log'
alias gco='git checkout' gb='git branch' gm='git merge' gst='git stash'
alias ga='git add' gmv='git mv' grm='git rm'
alias gc='git commit' gca='gc --amend' gt='git tag'
alias gp='git push' gu='git pull' gf='git fetch'
alias gr='git rebase' gra='gr --abort' grc='gr --continue' grs='gr --skip'

setopt prompt_subst
_prompt_git() {
  local dotgit=.git head
  [ -d "$dotgit" ] || dotgit=../.git
  [ -d "$dotgit" ] || return 0
  read head < "$dotgit/HEAD"
  case "$head" in
    ref:*) echo ":${head#*/*/}";;
    *) echo ":${head:0:7}";;
  esac
}
[ -n "$SSH_CLIENT" ] && _prompt_ssh='%F{magenta}'
PROMPT="%(?.%F{green}$_prompt_ssh.%F{red})»%f "
RPROMPT='%F{blue}%50<…<%~%F{yellow}$(_prompt_git)%f'

_n() { _n() { echo } }
_title() { print -Pn "\e]0;$1\a" }
_title_precmd() { _title "%1~" }
_title_preexec() { _title "%1~: $1" }
precmd_functions=(_n _title_precmd)
preexec_functions=(_title_preexec)
