hi clear
syntax reset
let colors_name = 'trivial'
let &t_Co = 8

hi Normal ctermbg=NONE ctermfg=NONE

hi ColorColumn ctermbg=Black
hi EndOfBuffer ctermfg=DarkGray
hi VertSplit cterm=NONE ctermbg=NONE ctermfg=DarkGray
hi LineNr ctermfg=DarkGray
hi MatchParen ctermbg=NONE ctermfg=DarkYellow
hi ModeMsg ctermfg=DarkGray
hi NonText ctermfg=DarkGray
hi Search ctermbg=NONE ctermfg=Yellow
hi StatusLine cterm=NONE ctermbg=Black ctermfg=LightGray
hi StatusLineNC cterm=NONE ctermbg=Black ctermfg=DarkGray
hi Folded ctermbg=Black ctermfg=DarkGray
hi Visual cterm=inverse ctermbg=NONE

hi Comment ctermfg=DarkBlue

hi! link Constant Normal
hi String ctermfg=DarkCyan
hi link Character String

hi! link Identifier Normal

hi Statement ctermfg=LightGray
hi link Operator Normal

hi PreProc ctermfg=DarkGreen

hi! link Type Normal
hi link StorageClass Statement
hi link Structure StorageClass
hi link Typedef Structure

hi! link Special Statement
hi SpecialComment ctermfg=LightBlue
hi SpecialKey ctermfg=DarkMagenta

hi Underlined ctermfg=NONE
hi Error ctermbg=NONE ctermfg=LightRed
hi SpellBad ctermbg=NONE ctermfg=DarkRed
hi! link Todo SpecialComment

" Language-specifics.

hi diffAdded ctermfg=Green
hi diffRemoved ctermfg=Red

hi link rustModPath Identifier

hi link rubyDefine Structure
hi link rubyStringDelimiter String
hi link rubySymbol String
