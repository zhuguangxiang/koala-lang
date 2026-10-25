// Koala Playground - Code execution and editing
document.addEventListener('DOMContentLoaded', function() {
    // Add Run buttons and make code blocks editable
    document.querySelectorAll('pre code.language-koala').forEach(function(block) {
        const pre = block.parentElement;
        
        // Create Edit button
        const editBtn = document.createElement('button');
        editBtn.textContent = 'Edit';
        editBtn.className = 'edit-btn';
        pre.style.position = 'relative';
        pre.appendChild(editBtn);
        
        // Make code block editable only when Edit button is clicked
        editBtn.addEventListener('click', function(e) {
            e.stopPropagation();
            if (pre.querySelector('textarea')) return;
            
            // Create overlay to prevent page scroll
            const overlay = document.createElement('div');
            overlay.className = 'edit-overlay';
            document.body.appendChild(overlay);
            
            // Disable page scroll during editing
            document.body.classList.add('code-editing');
            
            pre.classList.add('editing');
            const textarea = document.createElement('textarea');
            textarea.className = 'code-edit';
            textarea.value = block.textContent;
            textarea.spellcheck = false;
            
            block.style.display = 'none';
            pre.insertBefore(textarea, block.nextSibling);
            textarea.focus();
            
            // Prevent scroll on mouse events in textarea
            textarea.addEventListener('mousedown', function(e) {
                e.stopPropagation();
            });
            textarea.addEventListener('mousemove', function(e) {
                e.stopPropagation();
            });
            
            // Update Run button
            const runBtn = pre.querySelector('.run-btn');
            if (runBtn) {
                runBtn.onclick = async function() {
                    const code = textarea.value;
                    runBtn.disabled = true;
                    runBtn.textContent = '⏳';
                    
                    const resultPanel = pre.nextElementSibling;
                    if (resultPanel && resultPanel.classList.contains('run-result')) {
                        resultPanel.style.display = 'block';
                        resultPanel.className = 'run-result';
                        resultPanel.innerHTML = '<span class="result-label">Output</span>\nRunning...';
                    }
                    
                    try {
                        const resp = await fetch('/run', {
                            method: 'POST',
                            headers: {'Content-Type': 'application/json'},
                            body: JSON.stringify({code: code})
                        });
                        const data = await resp.json();
                        
                        if (resultPanel && resultPanel.classList.contains('run-result')) {
                            if (data.error) {
                                resultPanel.className = 'run-result error';
                                resultPanel.innerHTML = '<button class="close-btn" onclick="this.parentElement.style.display=\'none\'">&times;</button><span class="result-label">Error</span>\n' + data.error;
                            } else {
                                const output = data.output || '(no output)';
                                const isError = output.includes('error:') || output.includes('failed');
                                resultPanel.className = 'run-result' + (isError ? ' error' : '');
                                resultPanel.innerHTML = '<button class="close-btn" onclick="this.parentElement.style.display=\'none\'">&times;</button><span class="result-label">' + (isError ? 'Error' : 'Output') + '</span>\n' + output;
                            }
                        }
                    } catch (e) {
                        if (resultPanel && resultPanel.classList.contains('run-result')) {
                            resultPanel.className = 'run-result error';
                            resultPanel.innerHTML = '<button class="close-btn" onclick="this.parentElement.style.display=\'none\'">&times;</button><span class="result-label">Connection Error</span>\nCannot connect to server.';
                        }
                    } finally {
                        runBtn.disabled = false;
                        runBtn.textContent = '▶ Run';
                    }
                };
            }
            
            textarea.addEventListener('keydown', function(e) {
                if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
                    if (runBtn) runBtn.click();
                }
                if (e.key === 'Escape') {
                    textarea.remove();
                    block.style.display = '';
                    pre.classList.remove('editing');
                    // Remove overlay and re-enable page scroll
                    const overlay = document.querySelector('.edit-overlay');
                    if (overlay) overlay.remove();
                    document.body.classList.remove('code-editing');
                }
            });
        });
        
        const btn = document.createElement('button');
        btn.textContent = '▶ Run';
        btn.className = 'run-btn';
        
        const resultPanel = document.createElement('div');
        resultPanel.className = 'run-result';
        resultPanel.style.display = 'none';
        pre.parentNode.insertBefore(resultPanel, pre.nextSibling);
        
        btn.onclick = async function() {
            const code = block.textContent;
            btn.disabled = true;
            btn.textContent = '⏳';
            
            resultPanel.style.display = 'block';
            resultPanel.className = 'run-result';
            resultPanel.innerHTML = '<span class="result-label">Output</span>\nRunning...';
            
            try {
                const resp = await fetch('/run', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({code: code})
                });
                const data = await resp.json();
                
                if (data.error) {
                    resultPanel.className = 'run-result error';
                    resultPanel.innerHTML = '<button class="close-btn" onclick="this.parentElement.style.display=\'none\'">&times;</button><span class="result-label">Error</span>\n' + data.error;
                } else {
                    const output = data.output || '(no output)';
                    const isError = output.includes('error:') || output.includes('failed');
                    resultPanel.className = 'run-result' + (isError ? ' error' : '');
                    resultPanel.innerHTML = '<button class="close-btn" onclick="this.parentElement.style.display=\'none\'">&times;</button><span class="result-label">' + (isError ? 'Error' : 'Output') + '</span>\n' + output;
                }
            } catch (e) {
                resultPanel.className = 'run-result error';
                resultPanel.innerHTML = '<button class="close-btn" onclick="this.parentElement.style.display=\'none\'">&times;</button><span class="result-label">Connection Error</span>\nCannot connect to server.';
            } finally {
                btn.disabled = false;
                btn.textContent = '▶ Run';
            }
        };
        pre.style.position = 'relative';
        pre.appendChild(btn);
    });
    
    // Register "koala" as Swift for syntax highlighting
    document.querySelectorAll('code.language-koala').forEach(function(el) {
        el.classList.remove('language-koala');
        el.classList.add('language-swift');
    });
    if (typeof hljs !== 'undefined') hljs.highlightAll();
});
