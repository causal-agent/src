let &t_Co = 16
hi clear
if exists('syntax_on')
  syntax reset
endif

let colors_name = 'subtle'

hi Normal ctermbg=NONE ctermfg=NONE

hi ColorColumn ctermbg=Black
hi EndOfBuffer ctermfg=DarkGray
hi VertSplit cterm=NONE ctermbg=NONE ctermfg=DarkGray
hi LineNr ctermfg=DarkGray
hi MatchParen ctermbg=DarkGray ctermfg=White
hi ModeMsg ctermfg=DarkGray
hi NonText ctermfg=DarkGray
hi StatusLine cterm=NONE ctermbg=Black ctermfg=LightGray
hi StatusLineNC cterm=NONE ctermbg=Black ctermfg=DarkGray
hi Visual ctermbg=DarkGray ctermfg=NONE

hi Comment ctermfg=DarkYellow

hi Constant ctermfg=NONE
hi String ctermfg=LightBlue
hi Character ctermfg=LightBlue

hi Identifier cterm=NONE ctermfg=NONE

hi Statement ctermfg=LightGray

hi PreProc ctermfg=LightGray
hi Macro ctermfg=LightMagenta
hi PreCondit ctermfg=LightRed

hi Type ctermfg=NONE
hi StorageClass ctermfg=LightGray

hi Special ctermfg=LightGray
hi SpecialComment ctermfg=DarkYellow

hi Underlined ctermfg=NONE

hi Error ctermbg=NONE ctermfg=LightRed

hi Todo ctermbg=NONE ctermfg=Yellow

hi SpecialKey ctermfg=DarkGray

hi link rustModPath Identifier
