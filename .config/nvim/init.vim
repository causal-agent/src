set hidden
set shortmess=atI visualbell
set wildmode=list:longest

set ignorecase smartcase
set tabstop=4 expandtab shiftwidth=2 shiftround smartindent

set title
set ruler showcmd
set scrolloff=1
set number cursorline colorcolumn=80,100
set list listchars=tab:»·,trail:·

nmap <leader><leader> :nohlsearch<CR>

set splitbelow splitright
map <ScrollWheelUp> <C-Y>
map <ScrollWheelDown> <C-E>

set background=dark
let $NVIM_TUI_ENABLE_TRUE_COLOR = 1
let g:gruvbox_italic = 0
let g:gruvbox_invert_selection = 0
let g:gruvbox_vert_split = 'bg0'
colorscheme gruvbox

execute pathogen#infect()
