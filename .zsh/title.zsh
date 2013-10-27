function _title {
  echo -en "\033]0;$@\a"
}

function title {
  if [ -n "$1" ]; then
    _title_custom=1
    _title $@
  else
    unset _title_custom
    _title_precmd
  fi
}

function _title_preexec {
  [ -z "$_title_custom" ] && _title "$1"
}

function _title_precmd {
  [ -z "$_title_custom" ] && _title zsh
}

typeset -ga preexec_functions
typeset -ga precmd_functions
preexec_functions+='_title_preexec'
precmd_functions+='_title_precmd'
