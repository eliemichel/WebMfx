// https://stackoverflow.com/questions/14267781/sorting-html-table-with-javascript/49041392#49041392

const getCellValue = (tr, idx) => tr.children[idx].innerText || tr.children[idx].textContent;

const comparer = (idx, asc) => (a, b) => ((v1, v2) => 
    v1 !== '' && v2 !== '' && !isNaN(v1) && !isNaN(v2) ? v1 - v2 : v1.toString().localeCompare(v2)
    )(getCellValue(asc ? a : b, idx), getCellValue(asc ? b : a, idx));

function setupSpreadsheet(spreadsheet) {
    spreadsheet.querySelectorAll('th').forEach(th => th.addEventListener('click', function() {
        const table = th.closest('table');
        const tbody = table.querySelector('tbody');
        const columnIndex = Array.from(th.parentNode.children).indexOf(th);
        // Sort data
        Array.from(tbody.querySelectorAll('tr'))
            .sort(comparer(columnIndex, this.asc = !this.asc))
            .forEach(tr => tbody.appendChild(tr));
        // Highlight clicked th
        table.querySelectorAll('th').forEach(otherTh => otherTh.classList.remove("spreadsheet-active"));
        th.classList.add("spreadsheet-active");
    }));

    // Column resize
    const wrappers = spreadsheet.querySelectorAll('.content-wrapper');
    let dragged = null;
    function startDrag(event) {
        const td = event.target.parentNode;
        const table = td.closest('table');
        const rect = td.getBoundingClientRect();
        const x = event.clientX - rect.left;
        let columnIndex = Array.from(td.parentNode.children).indexOf(td);
        if (x > 4) columnIndex += 1;
        if (columnIndex == 0) return;
        const firstRow = table.querySelectorAll('tbody tr:nth-child(1) td');
        const columnFirstCell = firstRow[columnIndex - 1];
        const nextColumnFirstCell = firstRow[columnIndex];
        dragged = {
            columnIndex: columnIndex,
            startMouseX: event.clientX,
            columnFirstCell: columnFirstCell,
            nextColumnFirstCell: nextColumnFirstCell,
            table: table,
            startColumnWidth: columnFirstCell.getBoundingClientRect().width,
            startNextColumnWidth: nextColumnFirstCell ? nextColumnFirstCell.getBoundingClientRect().width : null,
            startTableWidth: table.getBoundingClientRect().width,
        };
        event.stopPropagation();
        event.preventDefault();
    }
    function doDrag(event) {
        let offset = event.clientX - dragged.startMouseX;
        let tableWidth = dragged.startTableWidth;
        let width = dragged.startColumnWidth + offset;
        let nextWidth = 0;
        if (width < 10) {
            offset = 10 - dragged.startColumnWidth;
            width = 10;
        }
        if (dragged.nextColumnFirstCell) {
            nextWidth = dragged.startNextColumnWidth - offset;
            if (nextWidth < 10) {
                offset = dragged.startNextColumnWidth - 10;
                nextWidth = 10;
                width = dragged.startColumnWidth + offset;
            }
        }

        dragged.columnFirstCell.width = `${width}px`;
        dragged.columnFirstCell.style.maxWidth = `${width}px`;
        if (dragged.nextColumnFirstCell) {
            dragged.nextColumnFirstCell.width = `${nextWidth}px`;
            dragged.nextColumnFirstCell.style.maxWidth = `${nextWidth}px`;
        } else {
            tableWidth += offset;
            dragged.table.width = `${tableWidth}px`;
        }
        
    }
    function stopDrag(event) {
        console.log("release");
        dragged = null;
        window.removeEventListener("mousemove", doDrag);
        window.removeEventListener("mouseup", stopDrag);
    };
    wrappers.forEach(div => div.addEventListener('mousedown', function(event) {
        startDrag(event);
        window.addEventListener("mousemove", doDrag);
        window.addEventListener("mouseup", stopDrag);
    }));

    // Allow text overflow in cells
    spreadsheet.querySelectorAll('.spreadsheet tbody tr:nth-child(1) td').forEach(td => {
        td.width = `${td.getBoundingClientRect().width}px`;
        td.style.maxWidth = td.width;
    });
    spreadsheet.classList.add('sized');
}

document.addEventListener('DOMContentLoaded', () => {
    document.querySelectorAll('.spreadsheet').forEach(setupSpreadsheet);
});

// Test data
pointPositionData = [
   -1,  -0.5,    -1.7776786088943481,
   1.7776786088943481,  -0.5,    -1,
   -1.7776786088943481, 0.5682, -1,
   1.7776786088943481,  0.5, -1,
   -1.7776786088943481, -0.5,    1,
   1.7776786088943481,  -0.5,    1,
   -1.7776786088943481, 0.5, 1,
   1.7776786088943481,  0.5, 1,
];
cornerPointData = [];
faceSizeData = [];
app.updateSpreadsheet(8, 12, 6, pointPositionData, cornerPointData, faceSizeData);
