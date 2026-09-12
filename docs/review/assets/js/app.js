const input = document.getElementById('search');
const units = [...document.querySelectorAll('.unit')];
const fns = [...document.querySelectorAll('.function')];

function filter() {
    if (!input) return;

    const q = input.value.trim().toLowerCase();

    units.forEach(s => {
        s.hidden = !!q && !s.textContent.toLowerCase().includes(q);
    });

    fns.forEach(d => {
        d.hidden = !!q && !d.textContent.toLowerCase().includes(q);

        if (q && !d.hidden)
            d.open = true;
    });

    const count = document.getElementById('count');

    if (count)
        count.textContent = q
            ? '匹配 ' + units.filter(s => !s.hidden).length + ' 个章节；清空可恢复全文'
            : '';
}

if (input) {
    input.addEventListener('input', filter);

    document.getElementById('clear').onclick = () => {
        input.value = '';
        filter();
    };

    document.getElementById('expand').onclick = () =>
        fns.forEach(x => x.open = true);

    document.getElementById('collapse').onclick = () =>
        fns.forEach(x => x.open = false);

    document.getElementById('print').onclick = () => {
        input.value = '';
        filter();
        fns.forEach(x => x.open = true);
        window.print();
    };
}

function follow() {
    if (input && input.value) {
        input.value = '';
        filter();
    }

    const id = decodeURIComponent(location.hash.slice(1));
    const target = document.getElementById(id);

    if (target) {
        if (target.nextElementSibling?.matches('details'))
            target.nextElementSibling.open = true;

        target.scrollIntoView({ block: 'start' });
    }
}

window.addEventListener('hashchange', follow);

document.querySelectorAll('a[href^="#"]').forEach(a =>
    a.addEventListener('click', () => {
        if (input) {
            input.value = '';
            filter();
        }
    })
);
