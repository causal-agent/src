unsetopt beep
setopt nomatch interactive_comments

HISTFILE=~/.history
HISTSIZE=SAVEHIST=5000
setopt inc_append_history hist_ignore_dups

bindkey -v
KEYTIMEOUT=1

autoload -Uz compinit && compinit
autoload colors && colors

PATH=$PATH:~/.bin
export EDITOR=vim
export CLICOLOR=1 # color ls output on OS X

[[ "$OSTYPE" =~ 'darwin' ]] && alias osx=true || alias osx=false
osx || alias ls='ls --color'
osx || alias grep='grep --color'
osx && alias rm='rm -v' || alias rm='rm -vI'
osx && alias gvim=mvim || alias gvim='gvim 2> /dev/null'

tn() { [ -n "$1" ] && tmux new -s "$1" || tmux new }
ta() { [ -n "$1" ] && tmux attach -t "$1" || tmux attach }

alias ga='git add'
alias gb='git branch'
alias gc='git commit'
alias gca='git commit --amend'
alias gcl='git clone'
alias gco='git checkout'
alias gd='git diff'
alias gl='git log --graph --pretty=log'
alias gm='git merge'
alias gmv='git mv'
alias gp='git push'
alias gr='git rebase'
alias grc='git rebase --continue'
alias grs='git rebase --skip'
alias gra='git rebase --abort'
alias grm='git rm'
alias gs='git status -sb'
alias gsh='git show'
alias gst='git stash'
alias gt='git tag'
alias gu='git pull'
alias gf='git fetch'

[[ -n "$SSH_CLIENT" ]] && _prompt_ssh_color="$fg[magenta]"

function zle-line-init zle-keymap-select {
  _prompt_vi_color=
  [[ "$KEYMAP" == "vicmd" ]] && _prompt_vi_color="$fg[yellow]"
  zle reset-prompt
}
zle -N zle-line-init
zle -N zle-keymap-select

_prompt_git_branch() {
  [[ -f .git/HEAD ]] || return 0
  local head
  read head < .git/HEAD
  case "$head" in
    ref:*)
      echo ":${head#*/*/}"
      ;;
    *)
      echo ":${head:0:7}"
      ;;
  esac
}

setopt prompt_subst
PROMPT='%{%(?.$fg[green]$_prompt_ssh_color.$fg[red])$_prompt_vi_color%}»%{$reset_color%} '
RPROMPT='%{$fg[blue]%}%-50<…<%~%{$fg[yellow]%}$(_prompt_git_branch)%{$reset_color%}'

_newline_precmd() { _newline_precmd() { echo } }

_title() {
  [[ "$TERM" =~ 'xterm' ]] && print -Pn "\e]0;$@\a"
}
[[ -n "$SSH_CLIENT" ]] && _title_host='%m:'
_title_preexec() { _title "$_title_host%1~: $1" }
_title_precmd() { _title "$_title_host%1~" }

typeset -ga preexec_functions
typeset -ga precmd_functions
preexec_functions+=(_title_preexec)
precmd_functions+=(_newline_precmd _title_precmd)

[[ -f ~/.nvm/nvm.sh ]] && source ~/.nvm/nvm.sh

if [[ -d /usr/local/share/chruby ]]; then
  source /usr/local/share/chruby/chruby.sh
  chruby ruby
fi

true
