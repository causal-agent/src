. ~/.shrc

alias ls='ls -p --color=auto'
alias grep='grep --color=auto'

_PS0=${PS0/$'\n'/}
unset PS0
RPS1=${RPS1/'\?'/'${?/#0/}'}

rprompt() {
	printf '%*s\r' $((COLUMNS - 1)) "${RPS1@P}"
}
PS1='\n\[${_PS0@P}$(rprompt)\]'"${PS1}"
