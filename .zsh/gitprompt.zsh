# Ported from http://sebastiancelis.com/2009/11/16/zsh-prompt-git-users/

function gitprompt_update
{
  unset __CURRENT_GIT_BRANCH
  unset __CURRENT_GIT_BRANCH_STATUS
  unset __CURRENT_GIT_BRANCH_IS_DIRTY

  local st="$(git status 2>/dev/null)"
  if [[ -n "$st" ]]; then
    local -a arr
    arr=(${(f)st})

    if [[ $arr[1] =~ 'Not currently on any branch.' ]]; then
      __CURRENT_GIT_BRANCH='no-branch'
    else
      __CURRENT_GIT_BRANCH="${arr[1][(w)4]}";
    fi

    if [[ $arr[2] =~ 'Your branch is' ]]; then
      if [[ $arr[2] =~ 'ahead' ]]; then
        __CURRENT_GIT_BRANCH_STATUS='ahead'
      elif [[ $arr[2] =~ 'diverged' ]]; then
        __CURRENT_GIT_BRANCH_STATUS='diverged'
      else
        __CURRENT_GIT_BRANCH_STATUS='behind'
      fi
    fi

    if [[ ! $st =~ 'nothing' ]]; then # Untracked files count as clean
      __CURRENT_GIT_BRANCH_IS_DIRTY='1'
    fi
  fi
}

# Changed around the formatting here a bunch
function gitprompt
{
  if [ -n "$__CURRENT_GIT_BRANCH" ]; then
    local s="%{${fg[yellow]}%}"
    if [ -n "$__CURRENT_GIT_BRANCH_IS_DIRTY" ]; then
      s+="⚡"
    else
      s+=":"
    fi
    s+="$__CURRENT_GIT_BRANCH"
    case "$__CURRENT_GIT_BRANCH_STATUS" in
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

    echo $s
  fi
}

function gitprompt_preexec
{
  case "$1" in
    g*) # Switched from git* to also detect my short aliases
      __EXECUTED_GIT_COMMAND=1
      ;;
  esac
}

function gitprompt_precmd
{
  if [ -n "$__EXECUTED_GIT_COMMAND" ]; then
    gitprompt_update
    unset __EXECUTED_GIT_COMMAND
  fi
}

typeset -ga preexec_functions
typeset -ga precmd_functions
typeset -ga chpwd_functions

preexec_functions+='gitprompt_preexec'
precmd_functions+='gitprompt_precmd'
chpwd_functions+='gitprompt_update'
