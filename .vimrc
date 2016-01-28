set nocompatible
set hidden
set ttimeoutlen=0
set shortmess=atI
set visualbell t_vb=
set backspace=indent,eol,start
set wildmenu wildmode=list:longest

set incsearch ignorecase smartcase hlsearch
set autoindent smartindent smarttab
set tabstop=4 expandtab shiftwidth=2 shiftround

set title
set ruler showcmd laststatus=2
set scrolloff=1
set number cursorline colorcolumn=80,100
set list listchars=tab:»·,trail:·
syntax on
filetype plugin indent on

noremap \ ,
let mapleader = ','
nmap <leader>n :nohlsearch<CR>

set guifont=ProFont:h11 guioptions=c

set background=dark
let g:gruvbox_termcolors = 16
let g:gruvbox_italic = 0
let g:gruvbox_invert_selection = 0
let g:gruvbox_sign_column = 'dark0'
let g:gruvbox_vert_split = 'dark0'
colorscheme gruvbox

execute pathogen#infect()
