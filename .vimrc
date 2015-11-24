set nocompatible

" Backspace past beginning of line in insert mode.
set backspace=indent,eol,start

" Allow switching buffers without saving.
set hidden

" Show cursor position and incomplete commands, always show status line.
set ruler showcmd laststatus=2

" Always show one extra line at the top or bottom of the window.
set scrolloff=1

" Search incrementally with smart case sensitivity, highlight all matches.
set incsearch ignorecase smartcase hlsearch

" Automatic indentation and adjust with tab and backspace.
set autoindent smartindent smarttab

" Show line numbers, highlight current line and fixed columns.
set number cursorline colorcolumn=80,100

" Set window title.
set title

" Shorten messages and disable intro screen
set shortmess=atI

" Disable audible bell.
set visualbell t_vb=

" Prevent delay when returning to norml mode in terminal vim.
set ttimeoutlen=0

" Show hard tabs and trailing whitespace
set list listchars=tab:»·,trail:·

" Show hard tabs as 4 side, use 2 space indentation rounded to multiples.
set tabstop=4 expandtab shiftwidth=2 shiftround

" Syntax highlighting, filetype indentation rules.
syntax on
filetype plugin indent on

" Show tab-complete suggestions and complete longest substring.
set wildmenu wildmode=list:longest

" Swap , and \ for leader.
noremap \ ,
let mapleader = ','

" Clear search results.
nmap <leader>n :nohlsearch<CR>

" Set GUI font and disable GUI features.
set guifont=ProFont:h11 guioptions=c

" Configure gruvbox colorscheme.
set background=dark
let g:gruvbox_termcolors = 16
let g:gruvbox_italic = 0
let g:gruvbox_invert_selection = 0
let g:gruvbox_sign_column = 'dark0'
let g:gruvbox_vert_split = 'dark0'
colorscheme gruvbox

execute pathogen#infect()
