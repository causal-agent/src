hi clear
syntax reset
let colors_name = 'trivial'
let &t_Co = 8

hi Normal ctermbg=NONE ctermfg=NONE

hi ColorColumn ctermbg=Black
hi EndOfBuffer ctermfg=DarkGray
hi VertSplit cterm=NONE ctermbg=NONE ctermfg=DarkGray
hi LineNr ctermfg=DarkGray
hi MatchParen ctermbg=NONE ctermfg=White
hi ModeMsg ctermfg=DarkGray
hi NonText ctermfg=DarkGray
hi Search ctermbg=NONE ctermfg=Yellow
hi StatusLine cterm=NONE ctermbg=Black ctermfg=LightGray
hi StatusLineNC cterm=NONE ctermbg=Black ctermfg=DarkGray
hi Folded ctermbg=Black ctermfg=DarkGray
hi Visual cterm=inverse ctermbg=NONE
hi Comment ctermfg=DarkBlue

hi Constant ctermfg=NONE
hi String ctermfg=DarkCyan
hi link Character String

hi Identifier cterm=NONE ctermfg=NONE

hi Statement ctermfg=LightGray

hi PreProc ctermfg=DarkGreen
hi Macro ctermfg=DarkYellow
hi link PreCondit Macro

hi Type ctermfg=NONE
hi StorageClass ctermfg=LightGray
hi link Structure StorageClass
hi link Typedef Structure

hi Special ctermfg=LightGray
hi SpecialComment ctermfg=LightBlue
hi SpecialKey ctermfg=DarkGray

hi Underlined ctermfg=NONE
hi Error ctermbg=NONE ctermfg=LightRed
hi Todo ctermbg=NONE ctermfg=LightBlue

" Language-specifics.

hi diffAdded ctermfg=Green
hi diffRemoved ctermfg=Red

hi link rustModPath Identifier

hi link rubyDefine Structure
hi link rubyStringDelimiter String
hi link rubySymbol String
