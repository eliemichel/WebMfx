// https://stackoverflow.com/questions/14267781/sorting-html-table-with-javascript/49041392#49041392

const getCellValue = (tr, idx) => tr.children[idx].innerText || tr.children[idx].textContent;

const comparer = (idx, asc) => (a, b) => ((v1, v2) => 
    v1 !== '' && v2 !== '' && !isNaN(v1) && !isNaN(v2) ? v1 - v2 : v1.toString().localeCompare(v2)
    )(getCellValue(asc ? a : b, idx), getCellValue(asc ? b : a, idx));

document.addEventListener('DOMContentLoaded', () => {
    document.querySelectorAll('.spreadsheet th').forEach(th => th.addEventListener('click', (() => {
        const table = th.closest('table');
        const tbody = table.querySelector('tbody');
        const columnIndex = Array.from(th.parentNode.children).indexOf(th);
        Array.from(tbody.querySelectorAll('tr'))
            .sort(comparer(columnIndex, this.asc = !this.asc))
            .forEach(tr => tbody.appendChild(tr));
    })));
});
