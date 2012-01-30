" Load pathogen
runtime bundle/vim-pathogen/autoload/pathogen.vim
call pathogen#infect()

" Remap leader to ,
noremap \ ,
let mapleader = ","

" General
set backspace=indent,eol,start
set hidden
set backup
set history=50
set ruler
set showcmd
set incsearch
set ignorecase
set smartcase
set autoindent
set smartindent
set nu
set spell
set cc=80
set title
set shortmess=atI
set visualbell t_vb=
set cursorline
set smarttab
set hlsearch
set list
set listchars=tab:»·,trail:·
set foldmethod=syntax
set foldlevel=1000

set laststatus=2 " Always show statusline
set statusline=%<%f\ %h%m%r%{fugitive#statusline()}%=%-14.(%l,%c%V%)\ %P

syntax on
colorscheme Tomorrow-Night

" Enable mouse in terminals
if has('mouse')
  set mouse=a
endif

" GUI options
set guioptions-=rLtT " Disable tear-off menus, toolbar, scrollbars
set guifont=Monospace\ 9
set browsedir=buffer " Open dialog starts in working directory
let g:Powerline_symbols = 'fancy'

" Jump to the last cursor position when opening
autocmd BufReadPost *
      \ if line("'\"") > 1 && line("'\"") <= line("$") |
      \   exe "normal! g`\"" |
      \ endif

" Default to 2-space indents, 4-character tabs
set expandtab
set shiftwidth=2
set tabstop=4
filetype plugin indent on

" Indentation exceptions
autocmd FileType c setlocal sw=4
autocmd FileType cpp setlocal sw=4

" Better tab-complete when opening
set wildmenu
set wildmode=list:longest
set wildignore=*.o,*.d,*~

" Smarter %
runtime macros/matchit.vim

" Disable visible whitespace in insert mode
autocmd InsertEnter * setlocal nolist
autocmd InsertLeave * setlocal list

" Syntastic options
let g:syntastic_check_on_open=1
let g:syntastic_enable_signs=0
let g:syntastic_auto_loc_list=2

" Custom maps
let g:buffergator_suppress_keymaps=1

nnoremap ' `
nnoremap ` '

" Clear search highlights
nmap <silent> <leader>n :silent :nohlsearch<CR>
" Toggle visible whitespace
nmap <silent> <leader>s :set list!<CR>

nmap <leader>p "+p
nmap <leader>P "+P
nmap <leader>y "+y
nmap <leader>Y "+Y
nmap <leader>d "+d
nmap <leader>D "+D

" Insert hard tab
imap <silent> <S-tab> <C-v><tab>

nmap Q gq

nmap <silent> <leader>b :CtrlPBuffer<CR>
nmap <silent> <leader>e :CtrlP<CR>

nmap <silent> <leader>gs :Gstatus<CR>
nmap <silent> <leader>gc :Gcommit<CR>
nmap <silent> <leader>gp :Git push<CR>

nnoremap <leader>u :GundoToggle<CR>

" Custom commands
command! W :w
