HISTFILE=~/.histfile
HISTSIZE=5000
SAVEHIST=5000
setopt appendhistory autocd extendedglob nomatch notify autopushd
setopt interactive_comments prompt_subst hist_ignore_dups
unsetopt beep
bindkey -v

# Completion
zstyle ':completion:*' completer _expand _complete _ignored _correct _approximate _prefix
zstyle ':completion:*' max-errors 2
zstyle :compinstall filename '/home/curtis/.zshrc'

autoload -Uz compinit
compinit

# Colors
autoload colors zsh/terminfo
colors

[[ -n "$COLORTERM" ]] && export TERM='xterm-256color'

# Libs

[[ -f /etc/zsh_command_not_found ]] && source /etc/zsh_command_not_found

source ~/.zsh/zsh-syntax-highlighting/zsh-syntax-highlighting.zsh
ZSH_HIGHLIGHT_HIGHLIGHTERS=(main brackets)
ZSH_HIGHLIGHT_STYLES[builtin]='none'
ZSH_HIGHLIGHT_STYLES[command]='bold'
ZSH_HIGHLIGHT_STYLES[precommand]='fg=yellow,bold'
ZSH_HIGHLIGHT_STYLES[alias]='fg=magenta,bold'
ZSH_HIGHLIGHT_STYLES[function]='fg=magenta,bold'
ZSH_HIGHLIGHT_STYLES[single-hyphen-option]='bold'
ZSH_HIGHLIGHT_STYLES[double-hyphen-option]='bold'
ZSH_HIGHLIGHT_STYLES[globbing]='fg=blue,bold'
ZSH_HIGHLIGHT_STYLES[path]='none'
ZSH_HIGHLIGHT_STYLES[history-expansion]='fg=blue,bold'
ZSH_HIGHLIGHT_STYLES[back-quoted-argument]='fg=cyan,bold'
ZSH_HIGHLIGHT_STYLES[dollar-double-quoted-argument]='fg=cyan,bold'
ZSH_HIGHLIGHT_STYLES[back-double-quoted-argument]='fg=cyan,bold'

source ~/.zsh/z/z.sh

if [[ -d /usr/local/share/chruby ]]; then
  source /usr/local/share/chruby/chruby.sh
  source /usr/local/share/chruby/auto.sh
  chruby 'ruby-2.0.0'
fi

[[ -s ~/.nvm/nvm.sh ]] && source ~/.nvm/nvm.sh

[[ -f /usr/local/heroku ]] && export PATH="/usr/local/heroku/bin:$PATH"

source ~/.zsh/gitprompt.zsh
source ~/.zsh/title.zsh

source ~/.zsh/aliases.zsh

# Environment

EDITOR=vim

# Prompt

unset _prompt_host
[[ -n "$SSH_CLIENT" ]] && _prompt_host="%{$fg[magenta]%}%m"
PROMPT=$'%{$terminfo[bold]%}$_prompt_host%{$fg[green]%}»%{$terminfo[sgr0]$reset_color%} '
RPROMPT=$'%{$terminfo[bold]%}%(?..%{$fg[red]%}%? )%{$fg[blue]%}%30<…<%~$(gitprompt)%{$terminfo[sgr0]%}'
