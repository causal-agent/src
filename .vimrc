" Do not try to behave like vi.
set nocompatible

" Allow backspace in insert mode to delete past the beginning of the line.
set backspace=indent,eol,start

" Keep buffers loaded even when they aren't shown. Allows switching buffers
" without saving first.
set hidden

" Create backup files before saving (file.txt is backed up to file.txt~).
set backup

" Keep 50 lines of : command history and search patterns.
set history=50

" Show the cursor position in the bottom right.
set ruler

" Show partial command in the bottom right (i.e. if a command is started but
" needs a motion, it will be shown).
set showcmd

" Jump to search results while typing. Pressing enter actually jumps to the
" result, pressing escape goes back to the cursor.
set incsearch

" Perform case-insensitive searching when the search pattern contains only
" lowercase letters.
set ignorecase
set smartcase

" Copy the indent from the previous line when starting a new line.
" Automatically indent between curly braces and indent keywords.
set autoindent
set smartindent

" Show line numbers.
set number

" Perform spell-checking on strings and comments.
set spell

" Highlight the 80th column.
set colorcolumn=80

" Set the window title with the current file name, status and directory.
set title

" Make file messages shorter:
"  - a: Shorten all file description messages
"  - t: Truncate file messages if they are too long
"  - I: Do not show the intro message when Vim starts
set shortmess=atI

" Disable beeping and visual bell (flashing the terminal window).
set visualbell t_vb=

" Highlight the current line.
set cursorline

" Insert `shiftwidth` spaces at the beginning of a line when tab is pressed,
" delete `shiftwidth` spaces when backspace is pressed.
set smarttab

" Highlight all search matches.
set hlsearch

" Show hard tabs and trailing whitespace.
set list
set listchars=tab:»·,trail:·


" Fold by syntax, start with all folds open.
set foldmethod=syntax
set foldlevelstart=99

" Always show the status line.
set laststatus=2

" Always show one line above or below the cursor.
set scrolloff=1

" Ctrl-A and Ctrl-X work on hex and single letters
set nrformats=alpha,hex

" Enable syntax highlighting.
syntax on

" Enable mouse in terminals
if has('mouse')
  set mouse=a
endif

" Disable spell-checking in terminal Vim.
if !has('gui_running')
  set nospell
endif

" GUI options:
"  * -m: Disable menu bar
"  * -r: Disable right scroll bar
"  * -L: Disable left scroll bar
"  * -T: Disable toolbar
"  * +c: Use console dialogs
set guioptions-=m
set guioptions-=r
set guioptions-=L
set guioptions-=T
set guioptions+=c

" Use a font.
set guifont=Monospace\ 9

" Jump to the last cursor position when opening a file.
au BufReadPost * if &filetype !~ '^git\c' && line("'\"") > 0 && line("'\"") <= line("$")
    \| exe "normal! g`\"" | endif

" Use two-space indents.
set expandtab
set shiftwidth=2

" Round to the nearest multiple of `shiftwidth` when indenting.
set shiftround

" Show hard tabs as 4 characters wide.
set tabstop=4


" Load filetype plugins and indentation rules.
filetype plugin indent on

" Use 4-space indents in C, C++ and Lua.
autocmd FileType c,cpp,lua setlocal sw=4

" Hard-wrap text at 72 characters in Markdown.
autocmd FileType markdown setlocal tw=72

" C/C++ indentation options:
"  * :0 Align `case` with `switch`
"  * l1 Indent case bodies with braces to case
"  * g0 Align `public:` and friends to class
set cinoptions=:0,l1,g0

" Show tab-complete suggestions when typing in the command-line. List all
" matches and complete to the longest common string. Ignore output files and
" backups.
set wildmenu
set wildmode=list:longest
set wildignore=*.o,*.d,*~

" Smarter % matching on HTML tags, if/endif etc.
runtime macros/matchit.vim

" Do not show whitespace in insert mode.
autocmd InsertEnter * setlocal nolist
autocmd InsertLeave * setlocal list

" Set leader to , and remap , to \.
noremap \ ,
let mapleader = ","

" Swap ' and ` (' is now character-wise and ` is line-wise).
nnoremap ' `
nnoremap ` '

" Clear search result highlighting.
nmap <leader>n :nohlsearch<CR>

" Toggle visible whitespace.
nmap <leader>s :set list!<CR>

" Toggle spell checking.
nmap <leader>z :set spell!<CR>

" Cut/copy/paste to system clipboard.
nmap <leader>p "+p
nmap <leader>P "+P
nmap <leader>y "+y
nmap <leader>Y "+Y
nmap <leader>d "+d
nmap <leader>D "+D

" Yank to end of line.
nmap Y y$

" Insert hard tab.
imap <S-tab> <C-v><tab>

" Toggle relative/absolute line numbers.
function! NumberToggle()
  if(&relativenumber == 1)
    set norelativenumber
  else
    set relativenumber
  endif
endfunc
nmap <C-n> :call NumberToggle()<CR>

" Common typos.
command! W :w
command! Q :q

" Plugins

call plug#begin('~/.vim/plugged')

" Fancy statusline.
Plug 'bling/vim-airline'
" Don't show mode in last line.
set noshowmode
" Disable silly > separators.
let g:airline_left_sep = ''
let g:airline_right_sep = ''
" Only show diff stats if there are some.
let g:airline#extensions#hunks#non_zero_only = 1
" Don't complain about whitespace constantly.
let g:airline#extensions#whitespace#enabled = 0

" Syntax checking.
Plug 'scrooloose/syntastic'
let g:syntastic_check_on_open=1
let g:syntastic_enable_signs=0

" Git diff signs in margins.
Plug 'mhinz/vim-signify'
let g:signify_vcs_list = ['git']
let g:signify_sign_overwrite = 1
let g:signify_sign_change = '~'

" Fuzzy matching files/buffers.
Plug 'kien/ctrlp.vim'
nmap <leader>b :CtrlPBuffer<CR>
nmap <leader>e :CtrlP<CR>
nmap <leader>t :CtrlPBufTag<CR>
nmap <leader>l :CtrlPLine<CR>

" Git commands.
Plug 'tpope/vim-fugitive'
nmap <leader>gs :Gstatus<CR>
nmap <leader>gc :Gcommit<CR>
nmap <leader>gp :Git push<CR>

" Alignment of = : , etc.
Plug 'junegunn/vim-easy-align'
vnoremap <silent> <Enter> :EasyAlign<Enter>

" Auto-close braces, parens, quotes, etc.
Plug 'Raimondi/delimitMate'
let delimitMate_expand_cr = 1
let delimitMate_expand_space = 1
let delimitMate_jump_expansion = 1

" Indent guides by alternating background colour.
Plug 'nathanaelkane/vim-indent-guides'
let g:indent_guides_start_level = 2

" Pastebin.
Plug 'Raynes/refheap.vim'
" Show nearest tag in statusline.
Plug 'majutsushi/tagbar'
" Pastebin.
Plug 'mattn/gist-vim'
" Required by gist.
Plug 'mattn/webapi-vim'
" Scratch buffers.
Plug 'programble/itchy.vim'
" Colorscheme.
Plug 'programble/jellybeans.vim'
" Better paste indentation.
Plug 'sickill/vim-pasta'
" Commenting.
Plug 'tpope/vim-commentary'
" Surround text objects.
Plug 'tpope/vim-surround'
" Sublime-style multiple cursors.
Plug 'terryma/vim-multiple-cursors'
" Increment, decrement dates and roman numerals with C-a, C-x.
Plug 'tpope/vim-speeddating'

" Language support.
Plug 'digitaltoad/vim-jade'
Plug 'groenewege/vim-less'
Plug 'kchmck/vim-coffee-script'
Plug 'pangloss/vim-javascript'
Plug 'tpope/vim-markdown'
Plug 'tpope/vim-ragtag'

call plug#end()

colorscheme jellybeans
