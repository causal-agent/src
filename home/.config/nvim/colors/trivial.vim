hi clear
syntax reset
let colors_name = 'trivial'

hi Normal ctermbg=NONE ctermfg=NONE

hi ColorColumn ctermbg=0
hi EndOfBuffer ctermfg=8
hi VertSplit cterm=NONE ctermbg=NONE ctermfg=8
hi LineNr ctermfg=8
hi MatchParen ctermbg=NONE ctermfg=3
hi ModeMsg ctermfg=8
hi NonText ctermfg=8
hi Search ctermbg=NONE ctermfg=11
hi StatusLine cterm=NONE ctermbg=0 ctermfg=7
hi StatusLineNC cterm=NONE ctermbg=0 ctermfg=8
hi Folded ctermbg=0 ctermfg=8
hi Visual cterm=inverse ctermbg=NONE

hi Comment ctermfg=4

hi! link Constant Normal
hi String ctermfg=6
hi link Character String

hi! link Identifier Normal

hi Statement ctermfg=7
hi link Operator Normal

hi PreProc ctermfg=2

hi! link Type Normal
hi link StorageClass Statement
hi link Structure StorageClass
hi link Typedef Structure

hi! link Special Normal
hi SpecialComment ctermfg=12
hi SpecialKey ctermfg=5

hi Underlined ctermfg=NONE
hi Error ctermbg=NONE ctermfg=9
hi SpellBad ctermbg=NONE ctermfg=1
hi! link Todo SpecialComment

" Language-specifics.

hi diffAdded ctermfg=10
hi diffRemoved ctermfg=9

hi link rustModPath Identifier

hi link rubyDefine Structure
hi link rubyStringDelimiter String
hi link rubySymbol String
