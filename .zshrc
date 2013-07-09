zstyle ':completion:*' completer _complete _ignored _correct _approximate
zstyle ':completion:*' max-errors 2
zstyle :compinstall filename '/home/curtis/.zshrc'

autoload -Uz compinit
compinit

HISTFILE=~/.histfile
HISTSIZE=5000
SAVEHIST=5000
setopt appendhistory autocd nomatch notify autopushd interactive_comments
setopt prompt_subst
unsetopt beep extendedglob

bindkey -e
bindkey "\e[1~" beginning-of-line
bindkey "\e[4~" end-of-line
bindkey "\e[5~" beginning-of-history
bindkey "\e[6~" end-of-history
bindkey "\e[3~" delete-char
bindkey "\e[2~" quoted-insert
bindkey "\e[5C" forward-word
bindkey "\eOc" emacs-forward-word
bindkey "\e[5D" backward-word
bindkey "\eOd" emacs-backward-word
bindkey "\e\e[C" forward-word
bindkey "\e\e[D" backward-word
bindkey "\e[8~" end-of-line
bindkey "\e[7~" beginning-of-line
bindkey "\eOH" beginning-of-line
bindkey "\eOF" end-of-line
bindkey "\e[H" beginning-of-line
bindkey "\e[F" end-of-line

autoload colors zsh/terminfo
colors

PROMPT=$'%{$terminfo[bold]$fg[green]%}[%{$fg[blue]%}%30<..<%~$(gitprompt)%{$fg[green]%}]%(!.#.$)%{$terminfo[sgr0]$reset_color%} '
RPROMPT="%(?..%{$terminfo[bold]$fg[green]%}[%{$fg[red]%}%?%{$fg[green]%}]%{$terminfo[sgr0]%})"

# Libs and stuff

source ~/.zsh/zsh-syntax-highlighting/zsh-syntax-highlighting.zsh
ZSH_HIGHLIGHT_STYLES[command]='bold'
ZSH_HIGHLIGHT_STYLES[builtin]='none'
ZSH_HIGHLIGHT_STYLES[alias]='fg=magenta,bold'
ZSH_HIGHLIGHT_STYLES[function]='fg=magenta,bold'
ZSH_HIGHLIGHT_STYLES[back-quoted-argument]='fg=yellow,bold'
ZSH_HIGHLIGHT_STYLES[single-hyphen-option]='bold'
ZSH_HIGHLIGHT_STYLES[double-hyphen-option]='bold'
ZSH_HIGHLIGHT_STYLES[globbing]='fg=blue,bold'
ZSH_HIGHLIGHT_STYLES[path]='none'
ZSH_HIGHLIGHT_STYLES[history-expansion]='fg=blue,bold'
ZSH_HIGHLIGHT_STYLES[dollar-double-quoted-argument]='fg=yellow,bold'
ZSH_HIGHLIGHT_STYLES[back-double-quoted-argument]='fg=yellow,bold'

source ~/.zsh/z/z.sh

source /usr/share/chruby/chruby.sh
source /usr/share/chruby/auto.sh
chruby ruby-2.0.0-p195

source ~/.zsh/gitprompt.zsh

# Environment

export EDITOR=vim
export PATH=$PATH:~/bin

[ "$TERM" = "xterm" ] && export TERM=xterm-256color

# Functions and aliases

function title {
  echo -en "\033]0;$1\a"
}

function game {
  xinit =$1 ${@:2} -- :1 vt6
}

function pacman {
  case $1 in
    -S | -S[^si]* | -R* | -U*)
      sudo /usr/bin/pacman "$@" ;;
    *)
      /usr/bin/pacman "$@" ;;
  esac
}

function mkcd {
  mkdir $@
  if [ "$1" = "-p" ]; then
    cd $2
  else
    cd $1
  fi
}

function reload {
  source ~/.zshrc
  reset
}

alias sprunge='curl -F "sprunge=<-" http://sprunge.us'

alias killlall='killall'
alias irb='ripl'
alias l='ls'
alias ll='ls'

alias ls='ls --color=auto'
alias grep='grep --color=auto'
alias rm='rm -vI'

alias S='pacman -S'
alias Syu='pacman -Syu'
alias Ss='pacman -Ss'
alias p='pacman'

compdef hub=git
alias git=hub
alias g=hub

alias ga='git add'
alias gb='git branch'
alias gc='git commit'
alias gcl='git clone'
alias gco='git checkout'
alias gd='git diff'
alias gi='git init'
alias gl='git log'
alias glg="git log --graph --pretty=format:'%Cred%h%Creset -%C(yellow)%d%Creset %s %Cgreen(%cr) %C(bold blue)<%an>%Creset' --abbrev-commit --date=relative --color"
alias gm='git merge'
alias gmv='git mv'
alias gp='git push'
alias gpom='git pull origin master'
alias gr='git remote'
alias grm='git rm'
alias gs='git status -sb'
alias gsh='git show'
alias gt='git tag'
alias gu='git pull'

[ -f /usr/bin/pacman ] && /usr/bin/pacman -Qu > /dev/null && echo "$(/usr/bin/pacman -Qu | wc -l) updates"
true
