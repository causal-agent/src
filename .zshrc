unsetopt beep
setopt nomatch interactive_comments
HISTFILE=~/.history
HISTSIZE=SAVEHIST=5000
setopt inc_append_history hist_ignore_dups

autoload -Uz compinit && compinit
autoload -Uz colors && colors

bindkey -v
KEYTIMEOUT=1

OLDPATH=$PATH
path=(
  /sbin
  /bin
  /usr/local/sbin
  /usr/local/bin
  /usr/sbin
  /usr/bin
  ~/.bin
  ~/.cargo/bin
  ~/.gem/bin
)

export PAGER=less MANPAGER=less EDITOR=vim GIT_EDITOR=vim
type nvim > /dev/null &&
  MANPAGER=manpager EDITOR=nvim GIT_EDITOR=nvim && alias vim=nvim
export GPG_TTY=$TTY

export CLICOLOR=1
[[ "$OSTYPE" = 'linux-gnu' ]] &&
  alias ls='ls --color' grep='grep --color' rm='rm -I'

alias gs='git status -sb'
alias ga='git add'
alias gc='git commit'
alias gca='git commit --amend'
alias gmv='git mv'
alias grm='git rm'
alias gst='git stash'
alias gt='git tag'
alias gsh='git show'
alias gco='git checkout'
alias gb='git branch'
alias gm='git merge'
alias gp='git push'
alias gf='git fetch'
alias gu='git pull'
alias gr='git rebase'
alias grc='git rebase --continue'
alias grs='git rebase --skip'
alias gra='git rebase --abort'
alias gd='git diff'
alias gl='git log --graph --pretty=log'

setopt prompt_subst
[[ -n "$SSH_CLIENT" ]] && _prompt_ssh='%F{magenta}'
_prompt_git() {
  local dotgit=.git head
  [[ -d "$dotgit" ]] || dotgit=../.git
  [[ -d "$dotgit" ]] || return 0
  read head < "$dotgit/HEAD"
  case "$head" in
    ref:*) echo ":${head#*/*/}";;
    *) echo ":${head:0:7}";;
  esac
}
PROMPT="%(?.%F{green}$_prompt_ssh.%F{red})»%f "
RPROMPT='%F{blue}%50<…<%~%F{yellow}$(_prompt_git)%f'

typeset -ga preexec_functions precmd_functions

_n() { _n() { echo } }
precmd_functions+=(_n)

_title() { print -Pn "\e]0;$@\a" }
_title_precmd() { _title "%1~" }
_title_preexec() { _title "%1~: $1" }
precmd_functions+=(_title_precmd)
preexec_functions+=(_title_preexec)

[[ -f ~/.nvm/nvm.sh ]] && source ~/.nvm/nvm.sh
if [[ -d /usr/local/share/chruby ]]; then
  source /usr/local/share/chruby/chruby.sh
  chruby ruby
fi

true
