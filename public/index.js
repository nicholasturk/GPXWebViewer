let refreshViewPanel = function() {
  $("#view-panel-table  .panel-comp").remove();
  let fileName = $(".file-dropdown.vp-file-dropdown")
    .find(":selected")
    .text();
  jQuery.ajax({
    type: "get",
    dataType: "json",
    url: `/fileInfo/${fileName}`,
    statusCode: {
      200: () => {
        console.log("OK");
      },
      500: data => {
        alert(data.responseText);
      }
    },
    success: function(data) {
      if (data.Route.length == 0 && data.Track.length == 0) {
        if ($(".no-comps").length === 0) {
          $(".view-panel-wrapper").append(
            "<h4 class='no-comps'>No components</h4>"
          );
        }
      } else {
        $(".no-comps").remove();
      }
      Object.keys(data).forEach(componentType => {
        data[componentType].forEach((component, idx) => {
          let $od = $("<div class='od'>");
          if (component.otherData.length == 0) {
            $od.text("None");
          } else {
            component.otherData.forEach(item => {
              let n = Object.keys(item)[0];
              $od.append(
                $("<div class='od-item hidden'>").html(
                  `<span class='border-bottom'><b>${n}</b></span><br><span>${item[n]}</span>`
                ),
                $(`<div class='show-od mt-2'>`).text("Show")
              );
            });
          }

          $(`<tr class='panel-comp'>`)
            .append(
              $("<td>").text(`${componentType} ${idx + 1}`),
              $("<td class='name-cell'>").html(
                `<input type="text" class="inp-name w-100" value="${component.name}"></input><button class='hidden change-name btn btn-sm btn-success mt-1'>Change</button>`
              ),
              $("<td>").text(component.numPoints),
              $("<td>").text(component.len),
              $("<td>").text(component.loop),
              $("<td class='od-cell'>").append($od)
            )
            .appendTo("#view-panel-table");
        });
        $("td").addClass("border-right");
      });
    }
  });
};

jQuery(document).ready(function() {
  jQuery.ajax({
    type: "get",
    dataType: "json",
    url: "/fileLogs",
    success: function(data) {
      if (data.length == 0) {
        $("<h4>")
          .text("No files available.")
          .appendTo(".file-log-wrapper");
      } else {
        data.forEach(file => {
          $("<tr>")
            .append(
              $("<td>").html(
                `<a href="${file.fileName}" download>${file.fileName}</a>`
              ),
              $("<td>").text(file.version),
              $("<td>").text(file.creator),
              $("<td>").text(file.numWaypoints),
              $("<td>").text(file.numRoutes),
              $("<td>").text(file.numTracks)
            )
            .appendTo("#file-log-table");
        });
        $("td").addClass("border-right");
      }
    },
    fail: function(error) {
      $("#blah").html("On page load, received error from server");
      console.log(error);
    }
  });

  jQuery.ajax({
    type: "get",
    dataType: "json",
    url: "/fileNames",
    success: function(data) {
      data.forEach(file => {
        $(`<option value=${file}>`)
          .text(file)
          .appendTo(".file-dropdown");
      });
    },
    fail: function(error) {
      $("#blah").html("On page load, received error from server");
      console.log(error);
    }
  });

  $("#uploadForm").submit(function(e) {
    e.preventDefault();
    var formData = new FormData($(this)[0]);
    $.ajax({
      type: "POST",
      url: "/upload",
      data: formData,
      contentType: false,
      processData: false,
      statusCode: {
        200: _ => {
          console.log("OK");
          window.location.reload();
        },
        400: data => {
          alert(data.responseText);
        },
        500: data => {
          alert(data.responseText);
        }
      }
    });
  });

  jQuery.ajax({
    type: "get",
    dataType: "json",
    url: "/getAllRoutes",
    success: function(data) {
      data.forEach(route => {
        $(`<option value=${route.route_name}>`)
          .text(route.route_name)
          .appendTo(".route-dropdown");
      });
    }
  });

  $("#loginForm").submit(function(e) {
    e.preventDefault();
    var formData = new FormData($(this)[0]);
    console.log(formData);
    $.ajax({
      type: "post",
      url: "/login",
      data: formData,
      contentType: false,
      processData: false,
      statusCode: {
        200: res => {
          console.log("OK");
          // window.location.reload();
          $(".is-logged-in").removeClass("hidden");
          $("#loginForm").remove();
          $(".loggedIn").text(`${res.user} connected to ${res.db}`);
          jQuery.ajax({
            type: "get",
            dataType: "json",
            url: "/getAllRoutes",
            success: function(data) {
              data.forEach(route => {
                $(`<option value=${route.route_name}>`)
                  .text(route.route_name)
                  .appendTo(".route-dropdown");
              });
            },
            fail: function(error) {
              $("#blah").html("On page load, received error from server");
              console.log(error);
            }
          });
        },
        400: data => {
          alert(data.responseText);
        },
        500: data => {
          alert(data.responseText);
        }
      }
    });
  });

  $("#addGPX").submit(function(e) {
    e.preventDefault();
    var formData = new FormData($(this)[0]);
    $.ajax({
      type: "POST",
      url: "/createGPX",
      data: formData,
      contentType: false,
      processData: false,
      statusCode: {
        200: _ => {
          console.log("OK");
          window.location.reload();
        },
        400: data => {
          alert(data.responseText);
        },
        500: data => {
          alert(data.responseText);
        }
      }
    });
  });

  $("#findPath").submit(function(e) {
    e.preventDefault();
    var formData = new FormData($(this)[0]);
    $.ajax({
      type: "POST",
      url: "/findPath",
      data: formData,
      contentType: false,
      processData: false,
      statusCode: {
        200: data => {
          console.log("OK");
          console.log(data);
          $(".found-comp").remove();
          $(".no-res-path").remove();
          if (data.length == 0) {
            $("<h5 class='no-res-path'>")
              .text("No results.")
              .appendTo("#find-path-wrapper");
          }
          data.forEach(file => {
            Object.keys(file).forEach(k => {
              console.log(k);
              if (k !== "fileName") {
                if (file[k].length > 0) {
                  file[k].forEach(comp => {
                    let $od = $("<div class='od'>");
                    if (comp.otherData.length == 0) {
                      $od.text("None");
                    } else {
                      comp.otherData.forEach(item => {
                        let n = Object.keys(item)[0];
                        $od.append(
                          $("<div class='od-item hidden'>").html(
                            `<span class='border-bottom'><b>${n}</b></span><br><span>${item[n]}</span>`
                          ),
                          $(`<div class='show-od mt-2'>`).text("Show")
                        );
                      });
                    }
                    $("<tr class='found-comp'>")
                      .append(
                        $("<td>").text(file["fileName"]),
                        $("<td>").text(k),
                        $("<td>").text(comp.name),
                        $("<td>").text(comp.numPoints),
                        $("<td>").text(comp.len),
                        $("<td>").text(comp.loop),
                        $("<td class='od-cell'>").append($od)
                      )
                      .appendTo("#find-path-table");
                  });
                }
              }
            });
          });
        },
        400: data => {
          alert(data.responseText);
        },
        500: data => {
          alert(data.responseText);
        }
      }
    });
  });

  $(".file-dropdown.vp-file-dropdown").change(() => {
    $("#view-panel-table  .panel-comp").remove();
    let fileName = $(".file-dropdown.vp-file-dropdown")
      .find(":selected")
      .val();
    if (fileName == "None") {
      return;
    }
    refreshViewPanel();
  });

  $(document).on("click", ".show-od", e => {
    $(e.target).text(
      $(e.target)
        .prev()
        .hasClass("hidden")
        ? "Hide"
        : "Show"
    );

    $(e.target)
      .prev()
      .toggleClass("hidden");
  });

  $(document).on("change paste keyup", ".inp-name", e => {
    if ($(e.target).val() != $(e.target).attr("value")) {
      $(e.target)
        .next()
        .removeClass("hidden");
    } else {
      $(e.target)
        .next()
        .addClass("hidden");
    }
  });

  $(document).on("click", ".change-name", e => {
    let fileName = $(".vp-file-dropdown")
      .find(":selected")
      .text();
    let newName = $(e.target)
      .prev()
      .val();
    let oldName = $(e.target)
      .prev()
      .attr("value");
    let componentType = $(e.target)
      .parent()
      .prev()
      .text()
      .split(" ")[0];
    if (newName == "") {
      alert("Name can not be empty.");
      return;
    }
    jQuery.ajax({
      type: "post",
      dataType: "json",
      url: "/changeComponentName",
      data: {
        fileName,
        componentType,
        oldName,
        newName
      },
      statusCode: {
        200: () => {
          console.log("success");
          refreshViewPanel();
        },
        500: data => {
          console.log(data.responseText);
          alert(data.responseText);
        }
      }
    });
  });

  $(document).on("click", ".add-wp", e => {
    $("#waypoints-list").append(
      $(`<div><input type="number" name="waypoints[][lat]" placeholder="Latitude" step="any" required/>
        <input
          class="mb-2"
          type="number"
          name="waypoints[][lon]"
          placeholder="Longitude"
          step="any"
          required
        /><button
          style="width:30px;"
          class="mb-1 pb-2 ml-1 w-10 btn btn-danger btn-sm del-wp"
        >
          x
        </button></div>`)
    );
  });

  $(document).on("click", ".add-gpx-btn", e => {
    if ($(e.target).hasClass("btn-primary")) {
      $(e.target).text("Add GPX");
    } else {
      $(e.target).text("Cancel");
    }
    $(e.target).toggleClass("btn-primary btn-danger");
    $("#add-gpx-wrapper").toggleClass("hidden");
  });

  $(document).on("click", ".add-route-btn", e => {
    if ($(e.target).hasClass("btn-primary")) {
      $(e.target).text("Add route");
    } else {
      $(e.target).text("Cancel");
    }
    $(e.target).toggleClass("btn-primary btn-danger");
    $("#add-route-wrapper").toggleClass("hidden");
  });

  $(document).on("click", ".find-path-btn", e => {
    if ($(e.target).hasClass("btn-primary")) {
      $(e.target).text("Find path");
    } else {
      $(e.target).text("Cancel");
    }
    $(e.target).toggleClass("btn-primary btn-danger");
    $("#find-path-wrapper").toggleClass("hidden");
  });

  $(document).on("click", ".del-wp", e => {
    $(e.target)
      .prev()
      .remove();
    $(e.target)
      .prev()
      .remove();
    $(e.target).remove();
  });

  $(document).on("click", "#store-all-files", e => {
    $.ajax({
      type: "get",
      url: "/storeAllFiles",
      contentType: false,
      processData: false,
      statusCode: {
        200: res => {
          $("#store-all-files + div").remove();
          $(
            `<div class="msg">Inserted ${res.numFiles} files.<br>Inserted ${res.numRoutes} routes.<br>Inserted ${res.numPoints} waypoints.</div>`
          ).insertAfter("#store-all-files");
          window.location.reload();
        },
        500: data => {
          alert(data.responseText);
        }
      }
    });
  });

  $(document).on("click", "#clear-all-data", e => {
    $.ajax({
      type: "get",
      url: "/clearAllData",
      contentType: false,
      processData: false,
      statusCode: {
        200: res => {
          $("#clear-all-data + div").remove();
          $(
            `<div class="msg">All rows removed from all tables.</div>`
          ).insertAfter("#clear-all-data");
        },
        500: data => {
          alert(data.responseText);
        }
      }
    });
  });

  $(document).on("click", "#display-db-status", e => {
    $.ajax({
      type: "get",
      url: "/displayDBStatus",
      contentType: false,
      processData: false,
      statusCode: {
        200: res => {
          alert(res);
        },
        500: data => {
          alert(data.responseText);
        }
      }
    });
  });

  $(document).on("change", "#query-select", e => {
    // $("#secdata").text(null);
    $(".sort-by-rts").addClass("hidden");
    $(".sort-by-pts").addClass("hidden");
    $(".sort-by-lonshor").addClass("hidden");
    $(".file-query-wrapper").addClass("hidden");
    let queryVal = $("#query-select")
      .find(":selected")
      .val();
    if (queryVal !== "0") {
      $("#query-submit").removeClass("hidden");
    } else {
      $("#query-submit").addClass("hidden");
    }
    $(".route-query-wrapper").addClass("hidden");
    if (queryVal === "0") {
      return;
    } else if (queryVal === "1") {
    } else if (queryVal === "2") {
      $(".file-query-wrapper").removeClass("hidden");
    } else if (queryVal === "3") {
      $(".route-query-wrapper").removeClass("hidden");
    } else if (queryVal === "4") {
      $(".file-query-wrapper").removeClass("hidden");
    } else if (queryVal === "5") {
      $(".file-query-wrapper").removeClass("hidden");
      $(".sort-by-lonshor").removeClass("hidden");
    }

    if (queryVal === "1" || queryVal === "2") {
      $(".sort-by-rts").removeClass("hidden");
    }

    if (queryVal === "4") {
      $(".sort-by-pts").removeClass("hidden");
    }
  });

  $("#query-form").submit(function(e) {
    e.preventDefault();
    let body = {};
    let queryVal = parseInt(
      $("#query-select")
        .find(":selected")
        .val()
    );
    let secData = null;
    let sortBy = "none";

    // Find sort
    if (queryVal === 1 || queryVal === 2) {
      sortBy = $(".sort-by-rts-q")
        .find(":selected")
        .val();
    } else if (queryVal === 4) {
      sortBy = $(".sort-by-pts-q")
        .find(":selected")
        .val();
    }

    // Find secondary data(file, route)
    if (queryVal === 2 || queryVal === 4 || queryVal === 5) {
      secData = $(".query-files")
        .find(":selected")
        .val();
    } else if (queryVal === 3) {
      secData = $(".route-dropdown")
        .find(":selected")
        .text();
    }

    // Unique stuff for query 5
    if (queryVal === 5) {
      body["larOrSmall"] = $(".lors-q")
        .find(":selected")
        .val();
      body["N"] = $("#5-n").val();
    }

    body["query"] = queryVal;
    body["sortBy"] = sortBy;
    body["sec"] = secData;

    $.ajax({
      type: "post",
      url: "/queryTable",
      dataType: "json",
      data: body,
      statusCode: {
        200: res => {
          $(".query-header").empty();
          $(".query-data").empty();
          console.log(res.secData);
          $("#secdata").text(res.secData);
          for (let col of res.columns) {
            $(".query-header").append($("<th>").text(col.name));
          }
          for (let row of res.rows) {
            let tr = $("<tr>");
            for (let k of Object.keys(row)) {
              tr.append($("<td>").text(row[k]));
            }
            $(".query-data").append($(tr));
          }
        },
        500: data => {
          alert(data.responseText);
        }
      }
    });
  });
});
