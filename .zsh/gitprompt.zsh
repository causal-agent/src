# Ported from http://sebastiancelis.com/2009/11/16/zsh-prompt-git-users/

function _gitprompt_update {
  unset _git_branch
  unset _git_status
  unset _git_dirty

  local st="$(git status 2> /dev/null)"
  if [[ -n "$st" ]]; then
    local -a arr
    arr=(${(f)st})

    if [[ $arr[1] =~ 'Not currently on any branch.' ]]; then
      _git_branch='none'
    else
      _git_branch="${arr[1][(w)-1]}"
    fi

    if [[ $arr[2] =~ 'Your branch is' ]]; then
      if [[ $arr[2] =~ 'ahead' ]]; then
        _git_status='ahead'
      elif [[ $arr[2] =~ 'diverged' ]]; then
        _git_status='diverged'
      else
        _git_status='behind'
      fi
    fi

    if [[ ! $st =~ 'nothing' ]]; then
      _git_dirty=1
    fi
  fi
}

function gitprompt {
  if [[ -n "$_git_branch" ]]; then
    local s
    [[ -z "$1" ]] && s="%{${fg[yellow]}%}"

    if [[ -n "$_git_dirty" ]]; then
      s+="⚡"
    else
      s+=":"
    fi

    s+="$_git_branch"
    case "$_git_status" in
      ahead)
        s+="↑"
        ;;
      diverged)
        s+="↕"
        ;;
      behind)
        s+="↓"
        ;;
    esac

    echo "$s"
  fi
}

function _gitprompt_preexec {
  [[ "$1" =~ "^g" ]] && _git_command=1
}

function _gitprompt_precmd {
  if [[ -n "$_git_command" ]]; then
    _gitprompt_update
    unset _git_command
  fi
}

typeset -ga preexec_functions
typeset -ga precmd_functions
typeset -ga chpwd_functions

preexec_functions+='_gitprompt_preexec'
precmd_functions+='_gitprompt_precmd'
chpwd_functions+='_gitprompt_update'
