# The following lines were added by compinstall

zstyle ':completion:*' completer _complete _ignored _correct _approximate
zstyle ':completion:*' max-errors 2
zstyle :compinstall filename '/home/curtis/.zshrc'

autoload -Uz compinit
compinit
# End of lines added by compinstall
# Lines configured by zsh-newuser-install
HISTFILE=~/.histfile
HISTSIZE=5000
SAVEHIST=5000
setopt appendhistory autocd nomatch notify
unsetopt beep extendedglob
bindkey -e
# End of lines configured by zsh-newuser-install

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

setopt autopushd
setopt interactive_comments

source /etc/profile.d/autojump.zsh

autoload colors zsh/terminfo
colors

source /etc/profile.d/pkgfile-hook.sh

PATH=$PATH:~/bin
export EDITOR=vim

export QEMU_AUDIO_DRV=alsa

[[ "$TERM" == "xterm" ]] && export TERM=xterm-256color 

function clyde {
	case $1 in
		-S | -S[^si]* | -R* | -U*)
			/usr/bin/sudo /usr/bin/clyde "$@" ;;
		*)
			/usr/bin/clyde "$@" ;;
	esac
}

function pacman {
	case $1 in
		-S | -S[^si]* | -R* | -U*)
			/usr/bin/sudo /usr/bin/pacman-color "$@" ;;
		*)
			/usr/bin/pacman-color "$@" ;;
	esac
}

mkcd() {
  mkdir $@
  if [ "$1" = "-p" ]; then
    cd $2
  else
    cd $1
  fi
}

function extract {
    echo Extracting $1 ...
    if [ -f $1 ] ; then
        case $1 in
            *.tar.bz2)   tar xjf $1  ;;
            *.tar.gz)    tar xzf $1  ;;
            *.bz2)       bunzip2 $1  ;;
            *.rar)       unrar e $1    ;;
            *.gz)        gunzip $1   ;;
            *.tar)       tar xf $1   ;;
            *.tbz2)      tar xjf $1  ;;
            *.tgz)       tar xzf $1  ;;
            *.zip)       unzip $1   ;;
            *.Z)         uncompress $1  ;;
            *.7z)        7z x $1  ;;
            *)           echo "'$1' cannot be extracted via extract()" ;;
        esac
    else
        echo "'$1' is not a valid file"
    fi
}

function reload {
    source ~/.zshrc
}

alias ls='ls --color=auto'
alias grep='grep --color=auto'
alias sprunge='curl -F "sprunge=<-" http://sprunge.us'
alias git=hub
compdef hub=git
alias readme='cat README*'
alias S='pacman -S'
alias Syu='pacman -Syu'
alias Ss='pacman -Ss'
alias p='pacman'
alias g='hub'
alias rm='rm -I'
alias tsmusic='ssh music@gewt.ath.cx'
alias gs='git status'
alias gc='git commit'
alias gd='git diff'
alias gp='git push'
alias gl='git log'
alias ga='git add'
alias gb='git branch'
alias gco='git checkout'
alias gm='git merge'
alias gcl='git clone'
alias gt='git tag'
alias gr='git remote'
alias gpl='git pull'
alias gsh='git show'
alias gmv='git mv'
alias grm='git rm'
alias gi='git init'
alias glg="git log --graph --pretty=format:'%Cred%h%Creset -%C(yellow)%d%Creset %s %Cgreen(%cr) %C(bold blue)<%an>%Creset' --abbrev-commit --date=relative --color"
alias mc='make clean'
alias m='make -j4'
alias killlall='killall'
alias irb='ripl'
alias l='ls'
alias t='task'

setopt PROMPT_SUBST

function prompt_task {
  COUNT=$(task count -longterm status.isnt:completed)
  [ "$COUNT" -gt 0 ] && echo "[%{$fg[red]%}$COUNT%{$fg[green]%}]"
}

PROMPT="%{$terminfo[bold]$fg[green]%}\$(prompt_task)[%{$fg[blue]%}%30<..<%~%{$fg[green]%}]%(!.#.$)%{$terminfo[sgr0]$reset_color%} "
RPROMPT="%(?..%{$terminfo[bold]$fg[green]%}[%{$fg[red]%}%?%{$fg[green]%}]%{$terminfo[sgr0]%})"

source ~/.zsh-syntax-highlighting/zsh-syntax-highlighting.zsh
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

pacman -Qu > /dev/null && [ ! -f /var/lib/pacman/db.lck ] && sudo pacman-color -Syu
echo -n
