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

function home.programble.me {
  ssh -R 8071:localhost:$1 quartz
}

alias killlall='killall'
alias irb='ripl'
alias l='ls'
alias ll='ls'

alias ls='ls --color=auto'
alias grep='grep --color=auto'
alias rm='rm -vI'

alias gvim='gvim 2> /dev/null'

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
alias gpom='git pull origin master'
alias gr='git remote'
alias grm='git rm'
alias gs='git status -sb'
alias gsh='git show'
alias gst='git stash'
alias gt='git tag'
alias gu='git pull'

alias hu='heroku'
alias gphu='git push heroku master'
