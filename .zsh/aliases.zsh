function reload {
  source ~/.zshrc
  rehash
}

function mkcd {
  mkdir $@
  if [ "$1" = "-p" ]; then
    cd $2
  else
    cd $1
  fi
}

function tunnel {
  ssh -R 8022:localhost:$1 do.asdf.pw
}

function vman {
  vim -c "SuperMan $*" || echo "No manual entry for $*"
}

alias randpasswd='openssl rand -base64 12'

alias killlall='killall'
which ripl &> /dev/null && alias irb='ripl'
alias l='ls'
alias ll='ls'

osx || alias ls='ls --color=auto'
osx || alias grep='grep --color=auto'
osx || alias rm='rm -vI'
osx && alias rm='rm -v'

osx || alias gvim='gvim 2> /dev/null'
osx && alias gvim='mvim'

alias b='bundle exec'
alias .env='export $(cat .env)'

alias sprunge='curl -F "sprunge=<-" http://sprunge.us'

if which hub &> /dev/null; then
  compdef hub=git
  alias git=hub
fi

alias g=git
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

alias hu='heroku'
alias gphu='git push heroku master'
