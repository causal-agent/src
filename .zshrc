unsetopt beep
setopt nomatch interactive_comments
HISTFILE=~/.history
HISTSIZE=SAVEHIST=5000
setopt inc_append_history hist_ignore_dups

autoload -Uz compinit && compinit
autoload -Uz colors && colors

bindkey -v
KEYTIMEOUT=1

PATH=$PATH:~/.bin
export EDITOR=vim

[[ "$OSTYPE" =~ 'darwin' ]] && alias osx=true || alias osx=false
osx && export CLICOLOR=1 || alias ls='ls --color' grep='grep --color'
osx || alias rm='rm -I'
osx && alias gvim=mvim || alias gvim='gvim 2> /dev/null'

alias gcl='git clone'
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
[[ -n "$SSH_CLIENT" ]] && _prompt_ssh="$fg[magenta]"
_prompt_git() {
  [[ -f .git/HEAD ]] || return 0
  local head
  read head < .git/HEAD
  case "$head" in
    ref:*) echo ":${head#*/*/}";;
    *) echo ":${head:0:7}";;
  esac
}
PROMPT='%{%(?.$fg[green]$_prompt_ssh.$fg[red])%}»%{$reset_color%} '
RPROMPT='%{$fg[blue]%}%-50<…<%~%{$fg[yellow]%}$(_prompt_git)%{$reset_color%}'

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
