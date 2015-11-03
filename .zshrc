# Save 5000 lines of history, writing after each command, ignoring duplicates.
HISTFILE=~/.history
HISTSIZE=SAVEHIST=5000
setopt inc_append_history hist_ignore_dups

# Error if glob does not match.
setopt nomatch

# Allow comments in interactive shell.
setopt interactive_comments

# No.
unsetopt beep

# Vim line editing.
bindkey -v

# Initialize completion.
autoload -Uz compinit && compinit

# Prompt with single character on the left, normally green, magenta over SSH,
# red after a failed command. Directory and git branch on the right.
setopt prompt_subst
autoload colors && colors
[[ -n "$SSH_CLIENT" ]] && _prompt_ssh_color="$fg[magenta]"
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
PROMPT='%{%(?.$fg[green]$_prompt_ssh_color.$fg[red])%}»%{$reset_color%} '
RPROMPT='%{$fg[blue]%}%-50<…<%~%{$fg[yellow]%}$(_prompt_git_branch)%{$reset_color%}'

# Print a newline before every prompt after the first one.
_newline_precmd() { _newline_precmd() { echo } }

# Set title to directory name at prompt, prefixed with hostname over SSH. Add
# current command to title while running.
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

# General environment setup.
PATH=$PATH:~/.bin
export EDITOR=vim
export CLICOLOR=1 # color ls output on OS X

# Detect OS X for conditional aliases.
[[ "$OSTYPE" =~ 'darwin' ]] && alias osx=true || alias osx=false

# Color output on Linux.
osx || alias ls='ls --color'
osx || alias grep='grep --color'

# Verbose output from rm with confirmation on Linux.
osx || alias rm='rm -vI'
osx && alias rm='rm -v'

# Suppress output from Linux gvim, alias gvim to MacVim.
osx || alias gvim='gvim 2> /dev/null'
osx && alias gvim=mvim

tn() { [ -n "$1" ] && tmux new -s "$1" || tmux new }
ta() { [ -n "$1" ] && tmux attach -t "$1" || tmux attach }

alias g=git
alias ga='git add'
alias gb='git branch'
alias gc='git commit'
alias gca='git commit --amend'
alias gcl='git clone'
alias gco='git checkout'
alias gd='git diff'
alias gi='git init'
alias gl='git log'
alias glg="git log --graph --pretty=format:'%Cred%h%Creset -%C(yellow)%d%Creset %s %Cgreen(%cr) %C(bold blue)<%an>%Creset' --abbrev-commit --date=relative --color"
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
alias gbl='git blame'

alias hu=heroku

[[ -f ~/.nvm/nvm.sh ]] && source ~/.nvm/nvm.sh

if [[ -d /usr/local/share/chruby ]]; then
  source /usr/local/share/chruby/chruby.sh
  chruby ruby
fi

# Prevent red first prompt.
true
